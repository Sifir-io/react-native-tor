@objc(Tor)
class Tor: NSObject {
    var service:Optional<OpaquePointer> = nil;
    
    func getProxiedClient()->URLSession{
        let config = URLSessionConfiguration.default;
        config.requestCachePolicy = URLRequest.CachePolicy.reloadIgnoringLocalCacheData;
        config.connectionProxyDictionary = [AnyHashable: Any]();
        config.connectionProxyDictionary?[kCFNetworkProxiesHTTPEnable as String] = 1;
        config.connectionProxyDictionary?[kCFStreamPropertySOCKSProxyHost as String] = "127.0.0.1";
        config.connectionProxyDictionary?[kCFStreamPropertySOCKSProxyPort as String] = 19032;
        config.connectionProxyDictionary?[kCFProxyTypeSOCKS as String] = 1;
        return URLSession.init(configuration: config, delegate: nil, delegateQueue: OperationQueue.current)
    }
    
    @objc(getOnionUrl:resolver:rejecter:)
    func getOnionUrl(url:String, resolve:@escaping RCTPromiseResolveBlock,reject:@escaping  RCTPromiseRejectBlock){
        let session = getProxiedClient();
        guard let _url = URL(string:url) else {
            // FIXME just to test
            reject("TOR","Could not parse url",NSError.init(domain: "TOR", code: 404));
            return;
        }
        do{
            let task = session.dataTask(with: _url) {  data, resp, error in
                guard let dataResp = data , error == nil else {
                    reject("TOR",error?.localizedDescription,error);
                    return;
                }
                resolve(dataResp.base64EncodedString());
                return;
            }
            task.resume();
        } catch{
            reject("TOR",error.localizedDescription,error);
            
        }
    }
    
    @objc(startDaemon:rejecter:)
    func startDaemon( resolve:RCTPromiseResolveBlock,reject:RCTPromiseRejectBlock)->Void{
        if service != nil {
            reject("TOR","Tor Service Already Running. Call `stopDaemon` first.",NSError.init(domain: "TOR", code: 99));
            return;
        }
        
        do {
            let temporaryDirectoryURL = URL(fileURLWithPath: NSTemporaryDirectory(),isDirectory: true)
            let socksPort:UInt16 = 19032;
            // this gives file:///Users/.../tmp/ so we remove the file:// prefix and trailing slash
            let path = try String(temporaryDirectoryURL.absoluteString.dropFirst(7).dropLast());
            let call_result = get_owned_TorService(path, socksPort).pointee;
            switch(call_result.message.tag){
            case Success:
                service = Optional.some(call_result.result);
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
            
        }
        catch {
            reject("TOR",error.localizedDescription,error);
        }
        
    }
    
    @objc(stopDaemon:rejecter:)
    func stopDaemon( resolve:RCTPromiseResolveBlock,reject:RCTPromiseRejectBlock)->Void {
        if let hasSevice = service {
            shutdown_owned_TorService(hasSevice);
            service = nil
        }
        resolve(true);
    }
}
