import Foundation;

// Trust SSLs even if invalid
extension Tor:URLSessionDelegate {
    public func urlSession(_ session: URLSession, didReceive challenge: URLAuthenticationChallenge, completionHandler: @escaping (URLSession.AuthChallengeDisposition, URLCredential?) -> Void) {
        //Trust the certificate even if not valid
        let urlCredential = URLCredential(trust: challenge.protectionSpace.serverTrust!)
        completionHandler(.useCredential, urlCredential)
    }
}

extension DispatchQueue {
    static func background(delay: Double = 0.0, background: (()->Void)? = nil, completion: (() -> Void)? = nil) {
        DispatchQueue.global(qos: .background).async {
            background?()
            if let completion = completion {
                DispatchQueue.main.asyncAfter(deadline: .now() + delay, execute: {
                    completion()
                })
            }
        }
    }
}




class ObserverSwift {
    public let onSuccess: ((String) -> Void)
    public let onError: ((String) -> Void)
    init(onSuccess: @escaping ((String) -> Void), onError: @escaping ((String) -> Void),target:String) {
        self.onSuccess = onSuccess
        self.onError = onError
    }
}

@objc(Tor)
class Tor: RCTEventEmitter {
    var service:Optional<OpaquePointer> = nil;
    var proxySocksPort:Optional<UInt16> = nil;
    var starting:Bool = false;
    var streams:Dictionary<String,OpaquePointer> = [:];
    var hiddenServices:Dictionary<String,OpaquePointer> = [:];
    var hasLnser = false;
    
    func getProxiedClient(headers:Optional<NSDictionary>,socksPort:UInt16,trustInvalidSSL: Bool = false)->URLSession{
        let config = URLSessionConfiguration.default;
        config.requestCachePolicy = URLRequest.CachePolicy.reloadIgnoringLocalCacheData;
        config.connectionProxyDictionary = [AnyHashable: Any]();
        config.connectionProxyDictionary?[kCFNetworkProxiesHTTPEnable as String] = 1;
        config.connectionProxyDictionary?[kCFStreamPropertySOCKSProxyHost as String] = "127.0.0.1";
        config.connectionProxyDictionary?[kCFStreamPropertySOCKSProxyPort as String] = socksPort;
        config.connectionProxyDictionary?[kCFProxyTypeSOCKS as String] = 1;
        
        if let headersPassed = headers {
            config.httpAdditionalHeaders = headersPassed as? [AnyHashable : Any]
        }
        if(trustInvalidSSL){
            return URLSession.init(configuration: config, delegate: self, delegateQueue: nil)
        } else {
            return URLSession.init(configuration: config, delegate: nil, delegateQueue: OperationQueue.current)
        }
    }
    
    func resolveObjResp (data:Data,resp:HTTPURLResponse,resolve: @escaping RCTPromiseResolveBlock,reject: @escaping RCTPromiseRejectBlock)->Void{
        let jsonObject: NSMutableDictionary = NSMutableDictionary()
        jsonObject.setValue(data.base64EncodedString(), forKey: "b64Data")
        jsonObject.setValue(resp.mimeType, forKey: "mimeType")
        jsonObject.setValue(resp.allHeaderFields, forKey: "headers")
        jsonObject.setValue(resp.statusCode, forKey: "respCode")
        // parse json if that's what we have
        if let mimeType = resp.mimeType {
            if mimeType == "application/json" || mimeType == "application/javascript" {
                do{
                    let json = try JSONSerialization.jsonObject(with: data, options: .allowFragments)
                    jsonObject.setValue(json, forKey: "json")
                } catch {
                    print("prepareObjResp errorParsingJson!",error);
                }
            }
        }
        
        
        if 200...299 ~= resp.statusCode {
            resolve(jsonObject as NSObject )
        }else{
            var msg:Optional<String> = nil;
            if let errorMessage = String(data:data,encoding: .utf8) {
                msg = errorMessage
            }
            reject(
                "TOR.REQUEST","Resp Code: \(resp.statusCode) : \(msg)",NSError.init(domain: "TOR.REQUEST", code: resp.statusCode,userInfo:["data" : msg]));
        }
    }
    
    @objc(request:method:jsonBody:headers:trustInvalidCert:resolver:rejecter:)
    func request(url: String, method: String, jsonBody: String, headers: NSDictionary, trustInvalidCert:Bool, resolve: @escaping RCTPromiseResolveBlock,reject: @escaping RCTPromiseRejectBlock){
        
        if service == nil {
            reject("TOR.SERVICE","Tor Service NOT Running. Call `startDaemon` first.",NSError.init(domain: "TOR.DAEMON", code: 99));
            return;
        }
        
        let session = getProxiedClient(headers:headers,socksPort: proxySocksPort!,trustInvalidSSL:trustInvalidCert);
        guard let _url = URL(string:url) else {
            reject("TOR.URL","Could not parse url",NSError.init(domain: "TOR", code: 404));
            return;
        }
        
        do{
            switch method{
            case "get":
                session.dataTask(with: _url) {  data, resp, error in
                    guard let dataResp = data , error == nil, let respData = resp else {
                        reject("TOR.NETWORK.GET",error?.localizedDescription,error);
                        return;
                    }
                    self.resolveObjResp(data:dataResp,resp:respData as! HTTPURLResponse,resolve:resolve,reject:reject);
                }.resume();
            case "delete":
                var request = URLRequest(url:_url);
                request.httpMethod = "DELETE";
                session.dataTask(with: request) {  data, resp, error in
                    guard let dataResp = data , error == nil, let respData = resp else {
                        reject("TOR.NETWORK.DELETE",error?.localizedDescription,error);
                        return;
                    }
                    self.resolveObjResp(data:dataResp,resp:respData as! HTTPURLResponse,resolve:resolve,reject:reject);
                    
                }.resume();
            case "post":
                var request = URLRequest(url:_url);
                request.httpMethod = "POST";
                session.uploadTask(with: request, from: jsonBody.data(using: .utf8)) {  data, resp, error in
                    guard let dataResp = data , let respData = resp , error == nil  else {
                        reject("TOR.NETWORK.POST",error?.localizedDescription,error);
                        return;
                    }
                    self.resolveObjResp(data:dataResp,resp:respData as! HTTPURLResponse,resolve:resolve,reject:reject);
                }.resume();
                
            default:
                throw NSError.init(domain:"TOR.REQUEST_METHOD",code:400)
            }
            
        } catch{
            reject("TOR.REQUEST",error.localizedDescription,error);
        }
    }
    
    
    @objc(startDaemon:resolver:rejecter:)
    func startDaemon(timeoutMs:NSNumber, resolve: @escaping RCTPromiseResolveBlock,reject: @escaping RCTPromiseRejectBlock)->Void{
        if service != nil || starting {
            reject("TOR.START","Tor Service Already Running. Call `stopDaemon` first.",NSError.init(domain: "TOR.START", code: 01));
            return;
        }
        Libsifir_ios.start_logger();
        starting = true;
        do {
            
            let temporaryDirectoryURL = URL(fileURLWithPath: NSTemporaryDirectory(),isDirectory: true)
            // FIXME pass this and check if avalible
            let socksPort:UInt16 = 19032;
            // this gives file:///Users/.../tmp/ so we remove the file:// prefix and trailing slash
            let path = String(temporaryDirectoryURL.absoluteString.dropFirst(7).dropLast());
            
            // Rust will start Tor daemon thread and block until boostrapped, so run as dispatched async task so not to block this thread
            DispatchQueue.background(background: {
                defer {
                    self.starting = false;
                }
                let call_result = get_owned_TorService(path, socksPort,timeoutMs.uint64Value).pointee;
                switch(call_result.message.tag){
                case Success:
                    self.service = Optional.some(call_result.result);
                    self.proxySocksPort = socksPort;
                    resolve(socksPort);
                    return;
                case Error:
                    // Convert RustByteSlice to String
                    if let error_body = call_result.message.error {
                        let error_string = String.init(cString: error_body);
                        reject("TOR.START",error_string,NSError.init(domain: "TOR", code: 0))
                    } else {
                        reject("TOR.START","Unknown daemon startup error",NSError.init(domain: "TOR", code: 99));
                    }
                    return;
                default:
                    reject("TOR.START","unknown startup result",NSError.init(domain: "TOR", code: 99));
                    return;
                }
            }, completion:{
            })
        }
    }
    
    @objc(getDaemonStatus:rejecter:)
    func getDaemonStatus(resolve:RCTPromiseResolveBlock,reject:RCTPromiseRejectBlock)->Void {
        guard let daemon = service else {
            if(starting)
            {
                resolve("STARTING")
            }else {
                resolve("NOTINIT");
            }
            return;
        }
        
        if let status = get_status_of_owned_TorService(daemon) {
            defer {
                destroy_cstr(status);
            }
            let status_string = String.init(cString: status);
            resolve(status_string)
        } else {
            reject("TOR.STATUS","UNKNOWN",NSError.init(domain: "TOR", code: 99));
        }
        
    }
    
    @objc(stopDaemon:rejecter:)
    func stopDaemon( resolve:RCTPromiseResolveBlock,reject:RCTPromiseRejectBlock)->Void {
        if let hasSevice = service {
            // if we have streams, shut them down
            for key in streams.keys {
                if let stream = streams[key] {
                    // set it here in case eof callback gets called
                    // while looping
                    streams[key] = nil;
                    tcp_stream_destroy(stream);
                }
            }
            shutdown_owned_TorService(hasSevice);
            service = nil
            proxySocksPort = nil
        }
        resolve(true);
    }
    
    override func startObserving(){
        self.hasLnser = true;
    }
    override func stopObserving(){
        self.hasLnser = false;
    }
    
    override func supportedEvents() -> [String]! {
        ["torTcpStreamData","torTcpStreamError","hsServiceHttpHandlerRequest","hsServiceHttpHandlerError"]
    }
    
    @objc(startTcpConn:timeoutMs:resolver:rejecter:)
    func startTcpConn(target:String,timeoutMs:NSNumber,resolve:RCTPromiseResolveBlock,reject:RCTPromiseRejectBlock){
        guard let socksProxy = self.proxySocksPort else {
            reject("TOR.TCPCONN.startTcpConn","SocksProxy not detected, make sure Tor is started",NSError.init(domain: "TOR", code: 99));
            return;
        }
        
        let uuid = UUID().uuidString;
        let call_result = tcp_stream_start(target, "127.0.0.1:\(socksProxy)",timeoutMs.uint64Value).pointee;
        
        switch(call_result.message.tag){
        case Success:
            let stream = call_result.result;
            self.streams[uuid] = stream;
            // Create swift observer wrapper to store context
            let observerWrapper = ObserverSwift(onSuccess:{ (data) in
                self.sendEvent(withName: "torTcpStreamData", body: "\(uuid)||\(data)")
            }, onError:{ (data) in
                // On Eof destrory stream and remove from map
                // TODO update this when streaming streams
                if(data == "EOF"){
                    guard let stream = self.streams[uuid] else{
                        print("Note: EOF but stream already destroyed, returning...")
                        return;
                    }
                    tcp_stream_destroy(stream);
                    self.streams[uuid] = nil;
                } else if (data.contains("NotConnected")){
                    guard self.streams[uuid] != nil else{
                        print("Note: EOF but stream already destroyed, returning...")
                        return;
                    }
                    // Stream pointer could be already delocated from rust side here
                    // so don't try to destory it just remove it from map of references.
                    // Worst case we create a new one and the memory leak gets recycled
                    // when app restarts
                    // TODO way to better coordinate this.
                    // tcp_stream_destroy(stream);
                    self.streams[uuid] = nil;
                } else {
                    print("Got observerWrapper event but not EOF",data )
                    
                }
                self.sendEvent(withName: "torTcpStreamError", body: "\(uuid)||\(data)")
            },target:target);
            // Prepare pointer to context and observer callbacks as Retained
            let owner = UnsafeMutableRawPointer(Unmanaged.passRetained(observerWrapper).toOpaque());
            
            let onSuccess:@convention(c) (UnsafeMutablePointer<Int8>?, UnsafeRawPointer?) -> Void = { (data, context) in
                // take unretained so we don't clear it
                let obv = Unmanaged<ObserverSwift>.fromOpaque(context!).takeUnretainedValue();
                obv.onSuccess(String(cString: data!));
                destroy_cstr(data);
                
            }
            let onError:@convention(c) (UnsafeMutablePointer<Int8>?, UnsafeRawPointer?) -> Void = { (data, context) in
                let obv = Unmanaged<ObserverSwift>.fromOpaque(context!).takeUnretainedValue();
                obv.onError(String(cString: data!));
                destroy_cstr(data);
            }
            let obv = Observer(context: owner, on_success: onSuccess, on_err:onError);
            tcp_stream_on_data(stream,obv);
            resolve(uuid);
            return;
        case Error:
            // Convert RustByteSlice to String
            if let error_body = call_result.message.error {
                let error_string = String.init(cString: error_body);
                reject("TOR.TCPCONN.startTcpConn",error_string,NSError.init(domain: "TOR", code: 0))
            } else {
                reject("TOR.TCPCONN.startTcpConn","Unknown tcpStream startup error",NSError.init(domain: "TOR", code: 99));
            }
            return;
        default:
            reject("TOR.startTcpConn","unknown startup result",NSError.init(domain: "TOR", code: 99));
            return;
        }
        
    }
    
    @objc(sendTcpConnMsg:msg:timeoutSec:resolver:rejecter:)
    func sendTcpConnMsg(target:String,msg:String,timeoutSec:NSNumber,resolve:RCTPromiseResolveBlock,reject:RCTPromiseRejectBlock){
        guard let _ = self.service else {
            reject("TOR.TCPCONN.sendTcpConnMsg","Service not detected, make sure Tor is started",NSError.init(domain: "TOR", code: 99));
            return;
        }
        guard let stream = self.streams[target] else{
            reject("TOR.TCPCONN.sendTcpConnMsg","Stream not detected",NSError.init(domain: "TOR", code: 99));
            return;
        }
        let result = tcp_stream_send_msg(stream, msg,timeoutSec.uint64Value).pointee;
        switch(result.tag){
        case Success:
            resolve(true);
            return;
        case Error:
            if let error_body = result.error {
                let error_string = String.init(cString: error_body);
                reject("TOR.TCPCONN.sendTcpConnMsg",error_string,NSError.init(domain: "TOR", code: 0))
            } else {
                reject("TOR.TCPCONN.sendTcpConnMsg","Unknown tcpStream startup error",NSError.init(domain: "TOR", code: 99));
            }
            return;
        default:
            reject("TOR.TCPCONN.sendTcpConnMsg","unknown tcp send message result",NSError.init(domain: "TOR", code: 99));
            return;
        }
    }
    
    @objc(stopTcpConn:resolver:rejecter:)
    func stopTcpConn(target:String,resolve:RCTPromiseResolveBlock,reject:RCTPromiseRejectBlock){
        guard let stream = self.streams[target] else{
            reject("TOR.TCPCONN.stopTcpConn","Stream not detected",NSError.init(domain: "TOR", code: 99));
            return;
        }
        self.streams[target] = nil;
        tcp_stream_destroy(stream);
        resolve(true);
    }
    
    @objc(createHiddenService:destinationPort:secretKey:resolver:rejecter:)
    func createHiddenService(hiddenServicePort:NSNumber,destinationPort:NSNumber,secretKey:String,resolve:RCTPromiseResolveBlock,reject:RCTPromiseRejectBlock){
        guard let _ = self.service else {
            reject("TOR.createHiddenService","Service not detected, make sure Tor is started",NSError.init(domain: "TOR", code: 99));
            return;
        }
        let result = create_hidden_service(self.service, destinationPort.uint16Value, hiddenServicePort.uint16Value,secretKey).pointee;
        switch(result.message.tag){
        case Success:
            guard let hs = result.result else {                  reject("TOR.HS.createHiddenService","missing result",NSError.init(domain: "TOR", code: 0))
                return;
            };
            defer {
                destroy_cstr(hs.pointee);
            }
            let hs_string =  String(cString: hs.pointee!);
            resolve(hs_string);
            return;
            
        case Error:
            if let error_body = result.message.error {
                let error_string = String.init(cString: error_body);
                reject("TOR.HS.createHiddenService",error_string,NSError.init(domain: "TOR", code: 0))
            } else {
                reject("TOR.HS.createHiddenService","Unknown hidden service creation error",NSError.init(domain: "TOR", code: 99));
            }
            return;
        default:
            reject("TOR.HS.createHiddenService","Unknown hidden service creation error",NSError.init(domain: "TOR", code: 99));
            return;
        }
    }
    
    @objc(deleteHiddenService:resolver:rejecter:)
    func deleteHiddenService(onion:String,resolve:RCTPromiseResolveBlock,reject:RCTPromiseRejectBlock){
        guard let _ = self.service else {
            reject("TOR.deleteHiddenService","Service not detected, make sure Tor is started",NSError.init(domain: "TOR", code: 99));
            return;
        }
        let result = delete_hidden_service(self.service, onion).pointee;
        switch(result.tag){
        case Success:
            resolve(true);
            return;

        case Error:
            if let error_body = result.error {
                let error_string = String.init(cString: error_body);
                reject("TOR.HS.deleteHiddenService",error_string,NSError.init(domain: "TOR", code: 0))
            } else {
                reject("TOR.HS.deleteHiddenService","Unknown hidden service creation error",NSError.init(domain: "TOR", code: 99));
            }
            return;
        default:
            reject("TOR.HS.deleteHiddenService","Unknown hidden service creation error",NSError.init(domain: "TOR", code: 99));
            return;
        }
    }
    
    
    @objc(startHttpHiddenserviceHandler:resolver:rejecter:)
    func startHttpHiddenserviceHandler(port:NSNumber,resolve:RCTPromiseResolveBlock,reject:RCTPromiseRejectBlock){
        let uuid = UUID().uuidString;
        
        let observerWrapper = ObserverSwift(onSuccess:{ (data) in
            self.sendEvent(withName: "hsServiceHttpHandlerRequest", body: "\(uuid)||\(data)")
        }, onError:{ (data) in
            self.sendEvent(withName: "hsServiceHttpHandlerError", body: "\(uuid)||\(data)")
        },target:uuid);
        
        // Prepare pointer to context and observer callbacks as Retained
        let owner = UnsafeMutableRawPointer(Unmanaged.passRetained(observerWrapper).toOpaque());
        
        let onSuccess:@convention(c) (UnsafeMutablePointer<Int8>?, UnsafeRawPointer?) -> Void = { (data, context) in
            // take unretained so we don't clear it
            let obv = Unmanaged<ObserverSwift>.fromOpaque(context!).takeUnretainedValue();
            obv.onSuccess(String(cString: data!));
            destroy_cstr(data);
        }
        let onError:@convention(c) (UnsafeMutablePointer<Int8>?, UnsafeRawPointer?) -> Void = { (data, context) in
            let obv = Unmanaged<ObserverSwift>.fromOpaque(context!).takeUnretainedValue();
            obv.onError(String(cString: data!));
            destroy_cstr(data);
        }
        let obv = Observer(context: owner, on_success: onSuccess, on_err:onError);
        
        let call_result = start_http_hidden_service_handler(port.uint16Value,obv).pointee;
        
        switch(call_result.message.tag){
        case Success:
            let hs = call_result.result;
            self.hiddenServices[uuid] = hs;
            resolve(uuid)
            return;
        case Error:
            // Convert RustByteSlice to String
            if let error_body = call_result.message.error {
                let error_string = String.init(cString: error_body);
                reject("TOR.HS.startHttpHiddenserviceHandler",error_string,NSError.init(domain: "TOR", code: 0))
            } else {
                reject("TOR.HS.startHttpHiddenserviceHandler","Unknown startHttpHiddenserviceHandler startup error",NSError.init(domain: "TOR", code: 99));
            }
            return;
        default:
            reject("TOR.startHttpHiddenserviceHandler","unknown startup result",NSError.init(domain: "TOR", code: 99));
            return;
            
        }
    }

    func stopHttpHiddenserviceHandler(id:String,resolve:RCTPromiseResolveBlock,reject:RCTPromiseRejectBlock){
        guard let hsh = self.hiddenServices[id] else{
            reject("TOR.HS.stopHttpHiddenserviceHandler","HiddenServiceHandler not detected",NSError.init(domain: "TOR", code: 99));
            return;
        }
        self.hiddenServices[id] = nil;
        destroy_hidden_service_handler(hsh);
        resolve(true);
    }
}
