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


@objc(Tor)
class Tor: NSObject {
    var service:Optional<OpaquePointer> = nil;
    var proxySocksPort:Optional<UInt16> = nil;
    var starting:Bool = false;

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
    
    func prepareObjResp (data:Data,resp:HTTPURLResponse,resolve: @escaping RCTPromiseResolveBlock,reject: @escaping RCTPromiseRejectBlock)->Void{
        let jsonObject: NSMutableDictionary = NSMutableDictionary()
        jsonObject.setValue(data.base64EncodedString(), forKey: "b64Data")
        jsonObject.setValue(resp.mimeType, forKey: "mimeType")
        jsonObject.setValue(resp.allHeaderFields, forKey: "headers")
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
            reject(
                "TOR.REQUEST","Resp Code: \(resp.statusCode)",NSError.init(domain: "TOR.REQUEST", code: resp.statusCode,userInfo: jsonObject as? [String : Any]));
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
                        reject("TOR.GET",error?.localizedDescription,error);
                        return;
                    }
                    self.prepareObjResp(data:dataResp,resp:respData as! HTTPURLResponse,resolve:resolve,reject:reject);
                }.resume();
            case "delete":
                var request = URLRequest(url:_url);
                request.httpMethod = "DELETE";
                session.dataTask(with: request) {  data, resp, error in
                    guard let dataResp = data , error == nil, let respData = resp else {
                        reject("TOR.DELETE",error?.localizedDescription,error);
                        return;
                    }
                    self.prepareObjResp(data:dataResp,resp:respData as! HTTPURLResponse,resolve:resolve,reject:reject);
                    
                }.resume();
            case "post":
                var request = URLRequest(url:_url);
                request.httpMethod = "POST";
                session.uploadTask(with: request, from: jsonBody.data(using: .utf8)) {  data, resp, error in
                    guard let dataResp = data , let respData = resp , error == nil  else {
                        reject("TOR.POST",error?.localizedDescription,error);
                        return;
                    }
                    self.prepareObjResp(data:dataResp,resp:respData as! HTTPURLResponse,resolve:resolve,reject:reject);
                }.resume();
                
            default:
                throw NSError.init(domain:"TOR.REQUEST_METHOD",code:400)
            }
            
        } catch{
            reject("TOR.REQUEST",error.localizedDescription,error);
        }
    }
    
    
    @objc(startDaemon:rejecter:)
    func startDaemon( resolve: @escaping RCTPromiseResolveBlock,reject: @escaping RCTPromiseRejectBlock)->Void{
        if service != nil || starting {
            reject("TOR.START","Tor Service Already Running. Call `stopDaemon` first.",NSError.init(domain: "TOR.START", code: 01));
            return;
        }
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
                let call_result = get_owned_TorService(path, socksPort).pointee;
                switch(call_result.message.tag){
                case Success:
                    self.service = Optional.some(call_result.result);
                    self.proxySocksPort = socksPort;
                    resolve(socksPort);
                    return;
                case Error:
                    // Convert RustByteSlice to String
                    if let error_body = call_result.message.error._0 {
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
            shutdown_owned_TorService(hasSevice);
            service = nil
            proxySocksPort = nil
        }
        resolve(true);
    }
}
