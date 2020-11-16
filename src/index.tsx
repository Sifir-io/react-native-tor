import { NativeModules } from 'react-native';

type TorType = {
  multiply(a: number, b: number): Promise<number>;
};

const { Tor } = NativeModules;

export default Tor as TorType;
