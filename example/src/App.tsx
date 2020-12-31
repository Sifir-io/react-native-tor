import * as React from 'react';
import { StyleSheet, View, Text, Button, TextInput } from 'react-native';
import TorBridge from 'react-native-tor';

const client = TorBridge();

export default function App() {
  const [socksPort, setSocksPort] = React.useState<number | undefined>();
  const [trustSSL, setTrustSSL] = React.useState<boolean>(true);
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
      let resp = await client.get(onion, undefined, trustSSL);
      console.log('got resp', resp);
    } catch (err) {
      console.error(err);
    }
  };
  const postOnion = async () => {
    try {
      if (!onion) throw 'No onion detected';
      let resp = await client.post(
        onion,
        JSON.stringify({ q: 'hello' }),
        {
          'Content-Type': 'application/json',
          // also supports
          // 'Content-Type': 'application/x-www-form-urlencoded',
        },
        trustSSL
      );
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
  // FIXME update this and test multiple message and lifescycle of connectin ?
  const sendTcpMsg = async () => {
    try {
      let target =
        'udfpzbte2hommnvag5f3qlouqkhvp3xybhlus2yvfeqdwlhjroe4bbyd.onion:60001';
      let msg =
        '{ "id": 1, "method": "blockchain.scripthash.get_balance", "params": ["716decbe1660861c3d93906cb1d98ee68b154fd4d23aed9783859c1271b52a9c"] }\n';
      let conn = await client.createTcpConnection({ target }, (data, err) => {
        console.log('tcp got msg', data, err);
      });
      await conn.write(msg);
      conn.close();
    } catch (err) {
      console.error('Error SendingTcpMSg', err);
    }
  };
  return (
    <View style={styles.container}>
      <View>
        <Button onPress={startTor} title="Start Tor">
          <Text>Start Tor</Text>
        </Button>
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
            <Button onPress={getOnion} title="Get onion" />
            <Button onPress={postOnion} title="POST onion" />
            <Button onPress={sendTcpMsg} title="Open,Send and Close TCP" />
            <View>
              <Text> Trust Self Signed SSL Toggle</Text>
              <Button
                onPress={() => setTrustSSL(!trustSSL)}
                title={
                  trustSSL ? 'Trusting Invalid SSL' : 'Not Trusting Invalid SSL'
                }
              />
            </View>
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
