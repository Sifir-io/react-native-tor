#import "React/RCTBridgeModule.h"
#import "React/RCTEventEmitter.h"

@interface RCT_EXTERN_REMAP_MODULE(TorBridge, Tor, RCTEventEmitter)

RCT_EXTERN_METHOD(
                  startDaemon:(RCTPromiseResolveBlock)resolve
                  rejecter: (RCTPromiseRejectBlock)reject
                  )
RCT_EXTERN_METHOD(
                  stopDaemon:(RCTPromiseResolveBlock)resolve
                  rejecter: (RCTPromiseRejectBlock)reject
                  )

RCT_EXTERN_METHOD(
                  getOnionUrl:(NSString*)url
                  resolver:(RCTPromiseResolveBlock)resolve
                  rejecter: (RCTPromiseRejectBlock)reject
                  )

RCT_EXTERN_METHOD(
                  getDaemonStatus:(RCTPromiseResolveBlock)resolve
                  rejecter: (RCTPromiseRejectBlock)reject
                  )

RCT_EXTERN_METHOD(
        request:(NSString*)url
        method:(NSString*)method
        jsonBody:(NSString*)jsonBody
        headers:(NSDictionary*)headers
        trustInvalidCert:(BOOL*)trustInvalidCert
        resolver:(RCTPromiseResolveBlock)resolve
        rejecter: (RCTPromiseRejectBlock)reject
)

RCT_EXTERN_METHOD(
        startTcpConn:(NSString*)target
        resolver:(RCTPromiseResolveBlock)resolve
        rejecter: (RCTPromiseRejectBlock)reject
)

RCT_EXTERN_METHOD(
        sendTcpConnMsg:(NSString*)target
        msg:(NSString*)msg
        resolver:(RCTPromiseResolveBlock)resolve
        rejecter: (RCTPromiseRejectBlock)reject
)

RCT_EXTERN_METHOD(
        stopTcpConn:(NSString*)target
        resolver:(RCTPromiseResolveBlock)resolve
        rejecter: (RCTPromiseRejectBlock)reject
)

@end
