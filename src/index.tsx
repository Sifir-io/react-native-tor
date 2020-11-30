import {
  NativeModules,
  AppState,
  AppStateStatus,
  Platform,
} from 'react-native';

type SocksPortNumber = number;
type RequestHeaders = { [header: string]: string } | {};
type ResponseHeaders = { [header: string]: string | string[] };
enum RequestMethod {
  'GET' = 'get',
  'POST' = 'post',
  'DELETE' = 'delete',
}

interface RequestBody {
  [RequestMethod.GET]: undefined;
  [RequestMethod.POST]: string;
  [RequestMethod.DELETE]: string | undefined;
}
interface RequestResponse<T = any> {
  mimeType: string;
  b64Data: string;
  headers: ResponseHeaders;
  json?: T;
}
interface ProcessedRequestResponse extends RequestResponse {}

interface NativeTor {
  startDaemon(): Promise<SocksPortNumber>;
  stopDaemon(): Promise<void>;
  getDaemonStatus(): Promise<string>;
  request<T extends RequestMethod>(
    url: string,
    method: T,
    data: string, // native side expects string for body
    headers: RequestHeaders
  ): Promise<RequestResponse>;
}
type TorType = {
  /** Shorthand for request for type GET **/
  get(url: string, headers?: RequestHeaders): Promise<ProcessedRequestResponse>;
  /** Shorthand for request for type POST **/
  post(
    url: string,
    body: RequestBody[RequestMethod.POST],
    headers?: RequestHeaders
  ): Promise<ProcessedRequestResponse>;
  /** Starts the TorDaemon if not started and returns a promise that fullfills with the socks port number when boostraping is compplete.
   * If the function was previously called it will return the promise without attempting to start the daemon again.
   * Useful when used as a guard in your transport or action layer
   */
  startIfNotStarted(): Promise<SocksPortNumber>;
  stopIfRunning(): Promise<void>;
  // FIXME to enum
  getDaemonStatus(): Promise<string>;
  request: NativeTor['request'];
};

const TorBridge: NativeTor = NativeModules.TorBridge;

export default ({
  stopDaemonOnBackground = true,
  startDaemonOnActive = false,
  os = Platform.OS,
} = {}): TorType => {
  let bootstrapPromise: Promise<number> | undefined;
  let lastAppState: AppStateStatus = 'active';
  let _appStateLsnerSet: boolean = false;
  const _handleAppStateChange = async (nextAppState: AppStateStatus) => {
    console.log('Got app state', nextAppState, lastAppState);
    if (
      startDaemonOnActive &&
      // lastAppState.match(/inactive|background/) &&
      lastAppState.match(/background/) &&
      nextAppState === 'active'
    ) {
      const status = NativeModules.TorBridge.getDaemonStatus();
      // Daemon should be in NOTINIT status if coming from background and this is enabled, so if not shutodwn and start again
      if (status !== 'NOTINIT') {
        stopIfRunning();
      }
      startIfNotStarted();
    }
    if (
      stopDaemonOnBackground &&
      lastAppState.match(/active/) &&
      nextAppState === 'background'
      // nextAppState === 'inactive'
    ) {
      const status = NativeModules.TorBridge.getDaemonStatus();
      if (status !== 'NOTINIT') {
        await stopIfRunning();
      }
    }
    lastAppState = nextAppState;
  };

  const startIfNotStarted = () => {
    if (!bootstrapPromise) {
      bootstrapPromise = NativeModules.TorBridge.startDaemon();
    }
    return bootstrapPromise;
  };
  const stopIfRunning = async () => {
    console.warn('Stopping Tor daemon.');
    bootstrapPromise = undefined;
    await NativeModules.TorBridge.stopDaemon();
  };

  /**
   * Post process request result
   * TODO make this injectable
   */
  const onAfterRequest = async (
    res: RequestResponse
  ): Promise<RequestResponse> => {
    if (os === 'android') {
      // Mapping JSONObject to ReadableMap for the bridge is a bit of a manual shitshow
      // so anroid JSON will be returned as string from the other side and we parse it here
      //
      if (res?.json) {
        const json = JSON.parse(res.json);
        return {
          ...res,
          json,
        };
      }
    }
    return res;
  };
  // Register app state lsner only once
  if (!_appStateLsnerSet) {
    AppState.addEventListener('change', _handleAppStateChange);
  }

  return {
    // FIXME i dont think we should export this here since it conflicts with out state managment
    // if someone calls start daemon directly on the Native module then the promise goes to shit..
    // ...TorBridge,
    async get(url: string, headers?: Headers) {
      await startIfNotStarted();
      return await onAfterRequest(
        await TorBridge.request(url, RequestMethod.GET, '', headers || {})
      );
    },
    async post(url: string, body: string, headers?: RequestHeaders) {
      await startIfNotStarted();
      return await onAfterRequest(
        await TorBridge.request(url, RequestMethod.POST, body, headers || {})
      );
    },
    async delete(url: string, body?: string, headers?: RequestHeaders) {
      await startIfNotStarted();
      return await onAfterRequest(
        await TorBridge.request(
          url,
          RequestMethod.DELETE,
          body || '',
          headers || {}
        )
      );
    },
    startIfNotStarted,
    stopIfRunning,
    request: TorBridge.request,
    getDaemonStatus: TorBridge.getDaemonStatus,
  } as TorType;
};
