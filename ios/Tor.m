#import "React/RCTBridgeModule.h"
#import "React/RCTEventEmitter.h"

@interface RCT_EXTERN_REMAP_MODULE(TorBridge, Tor, RCTEventEmitter)

RCT_EXTERN_METHOD(
                  startDaemon:(nonnull NSNumber*)timeoutMs
                  resolver:(RCTPromiseResolveBlock)resolver
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
                  createHiddenService:(nonnull NSNumber*)hiddenServicePort
                  destinationPort:(nonnull NSNumber*)destinationPort
                  secretKey:(NSString*)secretKey
                  resolver:(RCTPromiseResolveBlock)resolver
                  rejecter: (RCTPromiseRejectBlock)reject
                  )

RCT_EXTERN_METHOD(
                  deleteHiddenService:(NSString*)onion
                  resolver:(RCTPromiseResolveBlock)resolver
                  rejecter: (RCTPromiseRejectBlock)reject
                  )

RCT_EXTERN_METHOD(
                  startHttpHiddenserviceHandler:(nonnull NSNumber*)port
                  resolver:(RCTPromiseResolveBlock)resolver
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
        timeoutMs:(nonnull NSNumber*)timeoutMs
        resolver:(RCTPromiseResolveBlock)resolve
        rejecter: (RCTPromiseRejectBlock)reject
)

RCT_EXTERN_METHOD(
        sendTcpConnMsg:(NSString*)target
        msg:(NSString*)msg
        timeoutSec:(nonnull NSNumber*)timeoutSec
        resolver:(RCTPromiseResolveBlock)resolve
        rejecter: (RCTPromiseRejectBlock)reject
)

RCT_EXTERN_METHOD(
        stopTcpConn:(NSString*)target
        resolver:(RCTPromiseResolveBlock)resolve
        rejecter: (RCTPromiseRejectBlock)reject
)

@end
