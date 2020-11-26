import Foundation;

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

@objc(Tor)
class Tor: NSObject {
    var service:Optional<OpaquePointer> = nil;
    var proxySocksPort:Optional<UInt16> = nil;
    var starting:Bool = false;
    
    func getProxiedClient(headers:Optional<NSDictionary>,socksPort:UInt16)->URLSession{
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
        return URLSession.init(configuration: config, delegate: nil, delegateQueue: OperationQueue.current)
    }
    
    
    @objc(request:method:jsonBody:headers:resolver:rejecter:)
    func request(url: String, method: String, jsonBody: String, headers: NSDictionary,  resolve: @escaping RCTPromiseResolveBlock,reject: @escaping RCTPromiseRejectBlock){
        
        if service == nil {
            reject("TOR.SERVICE","Tor Service NOT Running. Call `startDaemon` first.",NSError.init(domain: "TOR.DAEMON", code: 99));
            return;
        }
        
        let session = getProxiedClient(headers:headers,socksPort: proxySocksPort!);
        guard let _url = URL(string:url) else {
            reject("TOR.URL","Could not parse url",NSError.init(domain: "TOR", code: 404));
            return;
        }
        do{
            switch method{
            case "get":
                session.dataTask(with: _url) {  data, resp, error in
                    guard let dataResp = data , error == nil else {
                        reject("TOR.RESP",error?.localizedDescription,error);
                        return;
                    }
                    resolve(dataResp.base64EncodedString());
                    return;
                }.resume();
            case "post":
                var request = URLRequest(url:_url);
                request.httpMethod = "POST";
                session.uploadTask(with: request, from: jsonBody.data(using: .utf8)) {  data, resp, error in
                    guard let dataResp = data , error == nil else {
                        reject("TOR.RESP",error?.localizedDescription,error);
                        return;
                    }
                    resolve(dataResp.base64EncodedString());
                    return;
                }.resume();
                
            default:
                throw NSError.init(domain:"TOR.METHOD",code:400)
            }
            
        } catch{
            reject("TOR.GENERAL",error.localizedDescription,error);
            
        }
        
    }
    
    @objc(startDaemon:rejecter:)
    func startDaemon( resolve: @escaping RCTPromiseResolveBlock,reject: @escaping RCTPromiseRejectBlock)->Void{
        if service != nil || starting {
            reject("TOR","Tor Service Already Running. Call `stopDaemon` first.",NSError.init(domain: "TOR", code: 99));
            return;
        }
    
        starting = true;
    
        do {
            let temporaryDirectoryURL = URL(fileURLWithPath: NSTemporaryDirectory(),isDirectory: true)
            
            // TODO pass this and check if avalible
            let socksPort:UInt16 = 19032;
            
            // this gives file:///Users/.../tmp/ so we remove the file:// prefix and trailing slash
            let path = try String(temporaryDirectoryURL.absoluteString.dropFirst(7).dropLast());
            
            DispatchQueue.background(background: {
                let call_result = get_owned_TorService(path, socksPort).pointee;
                switch(call_result.message.tag){
                case Success:
                    self.service = Optional.some(call_result.result);
                    self.proxySocksPort = socksPort;
                    resolve(socksPort);
                    return;
                case Error:
                    // Convert RustByteSlice to String
                    let error_body = call_result.message.error._0
                    let buffer = UnsafeBufferPointer(start: error_body.bytes, count: Int(error_body.len));
                    if let string = String(bytes: buffer, encoding: String.Encoding.utf8){
                        reject("TOR",string,NSError.init(domain: "TOR", code: 0))
                    } else {
                        reject("TOR","unknown",NSError.init(domain: "TOR", code: 99));
                    }
                    return;
                default:
                    reject("TOR","unknown-default",NSError.init(domain: "TOR", code: 99));
                    return;
                }
            }, completion:{
                self.starting = false;

            })
        }
        catch {
            reject("TOR",error.localizedDescription,error);
            starting = false;

        }
        
    }
    
    func rustBytesToString(msg:RustByteSlice)->Optional<String>{
        let buffer = UnsafeBufferPointer(start: msg.bytes, count: Int(msg.len));
        if let string = String(bytes: buffer, encoding: String.Encoding.utf8){
            return string;
        } else {
            return nil;
        }
    }
    
    @objc(getDaemonStatus:rejecter:)
    func getDaemonStatus(resolve:RCTPromiseResolveBlock,reject:RCTPromiseRejectBlock)->Void {
        if let daemon = service {
            let result = get_status_of_owned_TorService(daemon);
            
            defer {
                // FIXME call destory on rust side
                //result?.deallocate()
            }
            
            let call_result = result?.pointee
            switch(call_result?.message.tag){
            case Success:
                var  statusMessage:String = "UNKNOWN-SUCCESS";
                if let byteSlice = call_result?.result.pointee {
                    if let message = rustBytesToString(msg: byteSlice){
                        statusMessage = message;
                    }
                }
                resolve(statusMessage);
                return;
            case Error:
                var rejectMessage:String = "UNKNOWN-ERRO";
                if let errorBody = call_result?.message.error._0 {
                    if let message = rustBytesToString(msg: errorBody){
                        rejectMessage = message;
                    }
                }
                reject("TOR.STATUS",rejectMessage,NSError.init(domain: "TOR", code: 99));
                return;
            default:
                reject("TOR.STATUS","unknown-default",NSError.init(domain: "TOR", code: 99));
                return;

            }
            
        } else {
            if(starting)
            {
                resolve("STARTING")
            }else {
                resolve("NOTINIT");
            }
        }
    }
    
    @objc(stopDaemon:rejecter:)
    func stopDaemon( resolve:RCTPromiseResolveBlock,reject:RCTPromiseRejectBlock)->Void {
        if let hasSevice = service {
            shutdown_owned_TorService(hasSevice);
            service = nil
            proxySocksPort = nil
        }
        resolve(true);
    }
}
