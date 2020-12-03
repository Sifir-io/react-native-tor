import {
  NativeModules,
  AppState,
  AppStateStatus,
  Platform,
} from 'react-native';

type SocksPortNumber = number;
export type RequestHeaders = { [header: string]: string } | {};
export type ResponseHeaders = { [header: string]: string | string[] };
export enum RequestMethod {
  'GET' = 'get',
  'POST' = 'post',
  'DELETE' = 'delete',
}

export interface RequestBody {
  [RequestMethod.GET]: undefined;
  [RequestMethod.POST]: string;
  [RequestMethod.DELETE]: string | undefined;
}
export interface RequestResponse<T = any> {
  mimeType: string;
  b64Data: string;
  headers: ResponseHeaders;
  respCode: number;
  json?: T;
}
interface ProcessedRequestResponse extends RequestResponse {}

/**
 * Native module interface
 * Used internally, public calls should be made on the returned TorType
 */
interface NativeTor {
  startDaemon(): Promise<SocksPortNumber>;
  stopDaemon(): Promise<void>;
  getDaemonStatus(): Promise<string>;
  request<T extends RequestMethod>(
    url: string,
    method: T,
    data: string, // native side expects string for body
    headers: RequestHeaders,
    trustInvalidSSL: boolean
  ): Promise<RequestResponse>;
}
type TorType = {
  /**
   * Sen d a GET request routed through the SOCKS proxy on the native side
   * Starts the Tor Daemon automatically if not already started
   * @param url
   * @param headers
   * @param trustSSL
   */
  get(
    url: string,
    headers?: RequestHeaders,
    trustSSL?: boolean
  ): Promise<ProcessedRequestResponse>;
  /**
   * Send a POST request routed through the SOCKS proxy on the native side
   * Starts the Tor Daemon automatically if not already started
   * @param url
   * @param body
   * @param headers
   * @param trustSSL
   */
  post(
    url: string,
    body: RequestBody[RequestMethod.POST],
    headers?: RequestHeaders,
    trustSSL?: boolean
  ): Promise<ProcessedRequestResponse>;

  /**
   * Send a DELETE request routed through the SOCKS proxy on the native side
   * Starts the Tor Daemon automatically if not already started
   * @param url
   * @param headers
   * @param trustSSL
   */
  delete(
    url: string,
    body?: RequestBody[RequestMethod.DELETE],
    headers?: RequestHeaders,
    trustSSL?: boolean
  ): Promise<ProcessedRequestResponse>;
  /** Starts the Tor Daemon if not started and returns a promise that fullfills with the socks port number when boostraping is complete.
   * If the function was previously called it will return the promise without attempting to start the daemon again.
   * Useful when used as a guard in your transport or action layer
   */
  startIfNotStarted(): Promise<SocksPortNumber>;
  /**
   * Stops a running Tor Daemon
   */
  stopIfRunning(): Promise<void>;
  /**
   * Returns the current status of the Daemon
   * Some :
   * NOTINIT - Not initialized or run (call startIfNotStarted to the startDaemon)
   * STARTING - Daemon is starting and bootsraping
   * DONE - Daemon has completed boostraing and socks proxy is ready to be used to route traffic.
   * <other> - A status returned directly by the Daemon that can indicate a transient state or error.
   */
  getDaemonStatus(): Promise<string>;
  /**
   * Accessor the Native request function
   * Should not be used unless you know what you are doing.
   */
  request: NativeTor['request'];
};

const TorBridge: NativeTor = NativeModules.TorBridge;

/**
 * Tor module factory function
 * @param stopDaemonOnBackground
 * @default true
 * When set to true will shutdown the Tor daemon when the application is backgrounded preventing pre-emitive shutdowns by the OS
 * @param startDaemonOnActive
 * @default false
 * When set to true will automatically start/restart the Tor daemon when the application is bought back to the foreground (from the background)
 * @param os The OS the module is running on (Set automatically and is provided as an injectable for testing purposes)
 * @default The os the module is running on.
 */
export default ({
  stopDaemonOnBackground = true,
  startDaemonOnActive = false,
  os = Platform.OS,
} = {}): TorType => {
  let bootstrapPromise: Promise<number> | undefined;
  let lastAppState: AppStateStatus = 'active';
  let _appStateLsnerSet: boolean = false;
  const _handleAppStateChange = async (nextAppState: AppStateStatus) => {
    if (
      startDaemonOnActive &&
      lastAppState.match(/background/) &&
      nextAppState === 'active'
    ) {
      const status = NativeModules.TorBridge.getDaemonStatus();
      // Daemon should be in NOTINIT status if coming from background and this is enabled, so if not shutodwn and start again
      if (status !== 'NOTINIT') {
        await stopIfRunning();
      }
      startIfNotStarted();
    }
    if (
      stopDaemonOnBackground &&
      lastAppState.match(/active/) &&
      nextAppState === 'background'
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
   */
  const onAfterRequest = async (
    res: RequestResponse
  ): Promise<RequestResponse> => {
    if (os === 'android') {
      // Mapping JSONObject to ReadableMap for the bridge is a bit of a manual shitshow
      // so android JSON will be returned as string from the other side and we parse it here
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
    async get(url: string, headers?: Headers, trustSSL: boolean = true) {
      await startIfNotStarted();
      return await onAfterRequest(
        await TorBridge.request(
          url,
          RequestMethod.GET,
          '',
          headers || {},
          trustSSL
        )
      );
    },
    async post(
      url: string,
      body: RequestBody[RequestMethod.POST],
      headers?: RequestHeaders,
      trustSSL: boolean = true
    ) {
      await startIfNotStarted();
      return await onAfterRequest(
        await TorBridge.request(
          url,
          RequestMethod.POST,
          body,
          headers || {},
          trustSSL
        )
      );
    },
    async delete(
      url: string,
      body: RequestBody[RequestMethod.DELETE],
      headers?: RequestHeaders,
      trustSSL: boolean = true
    ) {
      await startIfNotStarted();
      return await onAfterRequest(
        await TorBridge.request(
          url,
          RequestMethod.DELETE,
          body || '',
          headers || {},
          trustSSL
        )
      );
    },
    startIfNotStarted,
    stopIfRunning,
    request: TorBridge.request,
    getDaemonStatus: TorBridge.getDaemonStatus,
  } as TorType;
};
