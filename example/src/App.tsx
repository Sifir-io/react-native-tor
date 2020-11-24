import * as React from 'react';
import { StyleSheet, View, Text, Button, TextInput } from 'react-native';
import TorBridge from 'react-native-tor';

export default function App() {
  const [socksPort, setSocksPort] = React.useState<number | undefined>();
  const [onion, setOnion] = React.useState<string | undefined>(
    'http://keybase5wmilwokqirssclfnsqrjdsi7jdir5wy7y7iu3tanwmtp6oid.onion'
  );

  React.useEffect(() => {
    startTor();
  }, []);

  const startTor = async () => {
    try {
      const port = await TorBridge.startDaemon();
      console.log('Tor started socks port', port);
      setSocksPort(port);
    } catch (err) {
      console.error(err);
    }
  };

  const stopTor = async () => {
    try {
      await TorBridge.stopDaemon();
    } catch (err) {
      console.error(err);
    }
    setSocksPort(undefined);
    console.log('Tor stopped');
  };
  const getOnion = async () => {
    try {
      if (!onion) throw 'No onion detected';
      let resp = await TorBridge.get(onion);
      console.log('got resp', resp);
    } catch (err) {
      console.error(err);
    }
  };
  return (
    <View style={styles.container}>
      <View>
        <Button onPress={startTor} title="Start Tor">
          <Text>Start Tor</Text>
        </Button>

        <Button onPress={stopTor} title="Stop Tor" disabled={!socksPort}>
          <Text>Stop Tor</Text>
        </Button>

        {!!socksPort && (
          <View>
            <TextInput
              style={{ height: 40, borderColor: 'gray', borderWidth: 1 }}
              onChangeText={setOnion}
              value={onion}
            />
            <Button onPress={getOnion} title="Get onion">
              <Text>Get Onion</Text>
            </Button>
          </View>
        )}
      </View>
      <Text>SocksPort: {socksPort}</Text>
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    alignItems: 'center',
    justifyContent: 'center',
  },
});
