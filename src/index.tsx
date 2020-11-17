import { NativeModules } from 'react-native';

type SocksPortNumber = number;
type TorType = {
  startDaemon(): Promise<SocksPortNumber>;
  stopDaemon(): Promise<void>;
};

const { Tor } = NativeModules;

export default Tor as TorType;
