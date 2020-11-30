import * as React from 'react';
import { StyleSheet, View, Text, Button, TextInput } from 'react-native';
import TorBridge from 'react-native-tor';

const client = TorBridge();

export default function App() {
  const [socksPort, setSocksPort] = React.useState<number | undefined>();
  const [onion, setOnion] = React.useState<string | undefined>(
    'http://3g2upl4pq6kufc4m.onion'
  );
  //const [clearnetJson ] = React.useState<string | undefined>(
  //  'https://api.jsonapi.co/rest/v1/user/list'
  //);
  React.useEffect(() => {
    _init();
  }, []);

  const _init = async () => {
    await client.startIfNotStarted();
  };
  const startTor = async () => {
    try {
      const port = await client.startIfNotStarted();
      console.log('Tor started socks port', port);
      setSocksPort(port);
    } catch (err) {
      console.error(err);
    }
  };

  const stopTor = async () => {
    try {
      await client.stopIfRunning();
    } catch (err) {
      console.error(err);
    }
    setSocksPort(undefined);
    console.log('Tor stopped');
  };
  const getOnion = async () => {
    try {
      if (!onion) throw 'No onion detected';
      let resp = await client.get(onion);
      console.log('got resp', resp);
    } catch (err) {
      console.error(err);
    }
  };
  const postOnion = async () => {
    try {
      if (!onion) throw 'No onion detected';
      let resp = await client.post(onion, JSON.stringify({ q: 'hello' }));
      console.log('got resp', resp);
    } catch (err) {
      console.error(err);
    }
  };
  const getStatus = async () => {
    try {
      let resp = await client.getDaemonStatus();
      console.log('got status', resp);
    } catch (err) {
      console.error('Error getDeamonStatus', err);
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

        <Button onPress={getStatus} title="Get Status">
          <Text>Get Daemon Status</Text>
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
            <Button onPress={postOnion} title="POST onion">
              <Text>Post To Onion</Text>
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
