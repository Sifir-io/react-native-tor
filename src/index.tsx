import { NativeModules } from 'react-native';

type SocksPortNumber = number;
// type JSONEncodedString = string;
type RequestHeaders = { [header: string]: any };
enum RequestMethod {
  'GET' = 'get',
  'POST' = 'post',
}

interface RequestBody {
  [RequestMethod.GET]: undefined;
  [RequestMethod.POST]: string;
}
type Base64EncodedBody = string;
type TorType = {
  startDaemon(): Promise<SocksPortNumber>;
  stopDaemon(): Promise<void>;
  getDaemonStatus():Promise<string>;
  // getOnionUrl(url: string): Promise<string>;
  request<T extends RequestMethod>(
    url: string,
    method: T,
    data: string, // native side expects string for body
    headers: RequestHeaders
  ): Promise<Base64EncodedBody>;
  /** Shorthand for request for type GET **/
  get(url: string, headers?: RequestHeaders): Promise<any>;
  /** Shorthand for request for type POST **/
  post(
    url: string,
    body: RequestBody[RequestMethod.POST],
    headers?: RequestHeaders
  ): Promise<any>;
};

const { TorBridge } = NativeModules;

export default {
  ...TorBridge,
  async get(url: string, headers?: Headers) {
    return TorBridge.request(url, RequestMethod.GET, '', headers || {});
  },
  async post(url: string, body: string, headers?: RequestHeaders) {
    return TorBridge.request(url, RequestMethod.POST, body, headers || {});
  },
} as TorType;
