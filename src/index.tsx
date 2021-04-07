import {
  NativeModules,
  DeviceEventEmitter,
  NativeEventEmitter,
  AppState,
  AppStateStatus,
  Platform,
  EmitterSubscription,
} from 'react-native';
import { queue } from 'async';

type SocksPortNumber = number;
export type RequestHeaders = { [header: string]: string } | {};
export type ResponseHeaders = { [header: string]: string | string[] };

/**
 * Supported Request types
 * @todo PUT
 */
export enum RequestMethod {
  'GET' = 'get',
  'POST' = 'post',
  'DELETE' = 'delete',
}

/**
 * Supported Body Payloads for the respective RequestMethod
 */
export interface RequestBody {
  [RequestMethod.GET]: undefined;
  [RequestMethod.POST]: string;
  [RequestMethod.DELETE]: string | undefined;
}

/**
 * Response returned from a successfully executed request
 */
export interface RequestResponse<T = any> {
  /**
   * Content mimeType returned by server
   */
  mimeType: string;
  /**
   * Base64 encoded string of data returned by server
   */
  b64Data: string;
  /**
   * String indexed object for headers returned by Server
   */
  headers: ResponseHeaders;
  /**
   * The response code for the request as returned by the server
   * Note: a respCode > 299 is considered an error by the client and throws
   */
  respCode: number;
  /**
   * If the mimeType of the payload is valid JSON then this field will
   * be populated with parsed JSON (object)
   */
  json?: T;
}
interface ProcessedRequestResponse extends RequestResponse {}

/**
 * Native module interface
 * Used internally, public calls should be made on the returned TorType
 */
interface NativeTor {
  startDaemon(timeoutMs: number): Promise<SocksPortNumber>;
  stopDaemon(): Promise<void>;
  getDaemonStatus(): Promise<string>;
  request<T extends RequestMethod>(
    url: string,
    method: T,
    data: string, // native side expects string for body
    headers: RequestHeaders,
    trustInvalidSSL: boolean
  ): Promise<RequestResponse>;
  startTcpConn(target: string, timeoutMs: number): Promise<boolean>;
  sendTcpConnMsg(
    target: string,
    msg: string,
    timeoutSeconds: number
  ): Promise<boolean>;
  stopTcpConn(target: string): Promise<boolean>;
}

/**
 * Tcpstream data handler.
 * If err is populated then there was an error
 */
type TcpConnDatahandler = (data?: string, err?: string) => void;

/**
 * Interface returned by createTcpConnection factory
 */
interface TcpStream {
  /**
   * Called to close and end the Tcp connection
   */
  close(): Promise<boolean>;
  /**
   * Send a message (write on socket)
   * @param msg
   */
  write(msg: string): Promise<boolean>;
}

/**
 * /**
 * Factory function to create a persistent TcpStream connection to a target
 * Wraps the native side emitter and subscribes to the targets data messages (string).
 * The TcpStream currently emits per line of data received . That is it reads data from the socket until a new line is reached, at which time
 * it will emit the data read (by calling onData(data,null). If an error is received or the connection is dropped it onData will be called
 * with the second parameter containing the error string (ie onData(null,'some error');
 * Note: Receiving an 'EOF' error from the target we're connected to signifies the end of a stream or the target dropped the connection.
 *       This will cause the module to drop the TcpConnection and remove all data event listeners.
 *       Should you wish to reconnect to the target you must initiate a new connection by calling createTcpConnection again.
 * @param param {target: string, writeTimeout: number, connectionTimeout: number } :
 *        `target` onion to connect to (ex: kciybn4d4vuqvobdl2kdp3r2rudqbqvsymqwg4jomzft6m6gaibaf6yd.onion:50001)
 *        'writeTimeout' in seconds to wait before timing out on writing to the socket (Defaults to 7)
 *        'connectionTimeout' in MilliSeconds to wait before timing out on connecting to the Target (Defaults to 15000 = 15 seconds)
 * @param onData TcpConnDatahandler node style callback called when data or an error is received for this connection
 * @returns TcpStream
 */
const createTcpConnection = async (
  param: { target: string; connectionTimeout?: number; writeTimeout?: number },
  onData: TcpConnDatahandler
): Promise<TcpStream> => {
  const { target } = param;
  const connectionTimeout = param.connectionTimeout || 15000;
  await NativeModules.TorBridge.startTcpConn(target, connectionTimeout);
  let lsnr_handle: EmitterSubscription[] = [];
  /**
   * Handles errors from Tcp Connection
   * Mainly check for EOF (connection closed/end of stream) and removes lnsers
   */
  const onError = async (event: string) => {
    if (event.toLowerCase() === 'eof') {
      console.warn(
        `Got to end of stream on TcpStream to ${target}. Removing listners`
      );
      try {
        await close();
      } catch (err) {
        console.warn('RnTor: onError close execution error', err);
      }
    }
  };
  if (Platform.OS === 'android') {
    lsnr_handle.push(
      DeviceEventEmitter.addListener(`${target}-data`, (event) => {
        onData(event);
      })
    );
    lsnr_handle.push(
      DeviceEventEmitter.addListener(`${target}-error`, async (event) => {
        await onError(event);
        await onData(undefined, event);
      })
    );
  } else if (Platform.OS === 'ios') {
    const emitter = new NativeEventEmitter(NativeModules.TorBridge);
    lsnr_handle.push(
      emitter.addListener(`torTcpStreamData`, (event) => {
        onData(event);
      })
    );
    lsnr_handle.push(
      emitter.addListener(`torTcpStreamError`, async (event) => {
        await onError(event);
        await onData(undefined, event);
      })
    );
  }
  const writeTimeout = param.writeTimeout || 7;
  const write = (msg: string) =>
    NativeModules.TorBridge.sendTcpConnMsg(target, msg, writeTimeout);
  const close = () => {
    lsnr_handle.map((e) => e.remove());
    return NativeModules.TorBridge.stopTcpConn(target);
  };
  return { close, write };
};

type TorType = {
  /**
   * Send a GET request routed through the SOCKS proxy on the native side
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

  /**
   * Factory function for creating a peristant Tcp connection to a target
   * See createTcpConnectio;
   */
  createTcpConnection: typeof createTcpConnection;
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
 * @param numberConcurrentRequests If sent to > 0 this will instruct the module to queue requests on the JS side before sending them over the Native bridge. Requests will get exectued with numberConcurrentRequests concurent requests. Note setting this to 0 disables JS sided queueing and sends requests directly to Native bridge as they are recieved. This is useful if you're running the stock/hermes RN JS engine that has a tendency off breaking under heavy multithreaded work. If you using V8 you can set this to 0 to disable JS sided queueing and thus get maximum performance.
 * @default 4
 * @param inflightRequestTimeoutMs The maximum number of MS to wait before timeing out a request that has been queued on the JS side has *begun* executing. Only used when numberConcurrentRequests > 0
 * @default 7000
 * @param os The OS the module is running on (Set automatically and is provided as an injectable for testing purposes)
 * @default The os the module is running on.
 */
export default ({
  stopDaemonOnBackground = true,
  startDaemonOnActive = false,
  bootstrapTimeoutMs = 25000,
  numberConcurrentRequests = 4,
  inflightRequestTimeoutMs = 7000,
  os = Platform.OS,
} = {}): TorType => {
  let bootstrapPromise: Promise<number> | undefined;
  let lastAppState: AppStateStatus = 'active';
  let _appStateLsnerSet: boolean = false;

  const requestQueue = queue<{
    res: (x: any) => void;
    rej: (x: any) => void;
    request: () => Promise<RequestResponse>;
  }>(async (task, cb) => {
    const { res, rej, request } = task;
    try {
      const timeout = setTimeout(() => {
        throw 'Request timedout after ' + inflightRequestTimeoutMs;
      }, inflightRequestTimeoutMs);
      const result = await request();
      clearTimeout(timeout);
      res(result);
    } catch (err) {
      rej(err);
    } finally {
      cb();
    }
  }, numberConcurrentRequests);

  requestQueue.drain(() =>
    console.log('notice: Request queue has been processed')
  );

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
      bootstrapPromise = NativeModules.TorBridge.startDaemon(
        bootstrapTimeoutMs
      );
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

  /**
   * Wraps requests to be queued or executed directly.
   * numberConcurrentRequests > 0 will cause tasks to be wrapped in a JS side queue
   */
  const requestQueueWrapper = (
    request: () => Promise<RequestResponse>
  ): Promise<RequestResponse> => {
    return new Promise((res, rej) =>
      numberConcurrentRequests > 0
        ? requestQueue.push({ request, res, rej })
        : request().then(res).catch(rej)
    );
  };

  return {
    async get(url: string, headers?: Headers, trustSSL: boolean = true) {
      await startIfNotStarted();
      return await onAfterRequest(
        await requestQueueWrapper(() =>
          TorBridge.request(url, RequestMethod.GET, '', headers || {}, trustSSL)
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
        await requestQueueWrapper(() =>
          TorBridge.request(
            url,
            RequestMethod.POST,
            body,
            headers || {},
            trustSSL
          )
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
        await requestQueueWrapper(() =>
          TorBridge.request(
            url,
            RequestMethod.DELETE,
            body || '',
            headers || {},
            trustSSL
          )
        )
      );
    },
    startIfNotStarted,
    stopIfRunning,
    request: TorBridge.request,
    getDaemonStatus: TorBridge.getDaemonStatus,
    createTcpConnection,
  } as TorType;
};
