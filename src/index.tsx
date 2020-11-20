import { NativeModules } from 'react-native';

type SocksPortNumber = number;
type TorType = {
  startDaemon(): Promise<SocksPortNumber>;
  stopDaemon(): Promise<void>;
  getOnionUrl(url: string): Promise<string>;
};

const { TorBridge } = NativeModules;

export default TorBridge as TorType;
