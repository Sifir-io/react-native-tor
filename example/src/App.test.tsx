import * as React from 'react';
import { StyleSheet, View, Text, Button, TextInput } from 'react-native';
import TorBridge from 'react-native-tor';

type Await<T> = T extends PromiseLike<infer U> ? U : T;
const client = TorBridge();
let tcpStream: Await<
  ReturnType<typeof client['createTcpConnection']>
> | null = null;

export default function App() {
  const [socksPort, setSocksPort] = React.useState<number | undefined>();
  const [trustSSL, setTrustSSL] = React.useState<boolean>(true);
  const [onion, setOnion] = React.useState<string | undefined>(
    'http://3g2upl4pq6kufc4m.onion'
  );
  const [hasStream, setHasStream] = React.useState(false);
  const [
    streamConnectionTimeoutMS,
    setStreamConnectionTimeoutMS,
  ] = React.useState(15000);
  React.useEffect(() => {
    _init();
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, []);

  const _init = async () => {
    try {
      // Init and do a few basic calls to test all is good
      console.log('App init');
      await client.startIfNotStarted();
      console.log('Tor started');
      await getOnion();
      console.log('Got onion');
      await startTcpStream();
      console.log('started tcp stream');
      await sendTcpMsg();
      console.log('send tcpmsg');
      await closeTcpStream();
      console.log('tcp closed');
      await startTcpStream(15000);
      await sendTcpMsg();
      console.log('sent 2');
      await stopTor();
      console.log('tor stop');
    } catch (err) {
      console.error('Error starting daemon', err);
      await client.stopIfRunning();
    }
  };
  const startTor = async () => {
    try {
      const port = await client.startIfNotStarted();
      console.log('Tor started socks port', port);
      setSocksPort(port);
    } catch (err) {
      await client.stopIfRunning();
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
  const sendTcpMsg = async () => {
    try {
      let msg = `{ "id": 1, "method": "blockchain.scripthash.get_balance", "params": ["716decbe1660861c3d93906cb1d98ee68b154fd4d23aed9783859c1271b52a9c"] }\n`;
      if (!tcpStream) throw 'Stream not set';
      await tcpStream.write(msg);
    } catch (err) {
      console.error('Error SendingTcpMSg', err);
    }
  };
  const startTcpStream = async (timeoutMs?: number) => {
    try {
      let target =
        'kciybn4d4vuqvobdl2kdp3r2rudqbqvsymqwg4jomzft6m6gaibaf6yd.onion:50001';
      console.log('starting');
      const conn = await client.createTcpConnection(
        { target, connectionTimeout: timeoutMs || streamConnectionTimeoutMS },
        (data, err) => {
          console.log('tcp got msg', data, err);
        }
      );
      console.log('after');
      tcpStream = conn;
      setHasStream(true);
    } catch (err) {
      console.error('Error StartingTcpStream', err);
    }
  };
  const closeTcpStream = async () => {
    try {
      if (!tcpStream) throw 'Stream not set';
      await tcpStream.close();
      tcpStream = null;
      setHasStream(false);
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
            <Button
              onPress={() => startTcpStream(streamConnectionTimeoutMS)}
              title="Start Tcp Stream"
            />
            <TextInput
              style={{ height: 40, borderColor: 'gray', borderWidth: 1 }}
              onChangeText={(x) => setStreamConnectionTimeoutMS(Number(x))}
              value={String(streamConnectionTimeoutMS)}
            />
            {hasStream && (
              <View>
                <Button onPress={sendTcpMsg} title="Send Tcp Msg" />
                <Button onPress={closeTcpStream} title="Close Tcp Stream" />
              </View>
            )}
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
