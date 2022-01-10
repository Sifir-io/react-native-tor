import * as React from 'react';
import { StyleSheet, View, Text, Button, TextInput, Alert } from 'react-native';
import TorBridge from 'react-native-tor';

type Await<T> = T extends PromiseLike<infer U> ? U : T;
const client = TorBridge();
let tcpStream: Await<ReturnType<typeof client['createTcpConnection']>> | null =
  null;

export default function App() {
  const [socksPort, setSocksPort] = React.useState<number | undefined>();
  const [trustSSL, setTrustSSL] = React.useState<boolean>(true);
  const [onion, setOnion] = React.useState<string | undefined>(
    'http://duckduckgogg42xjoc72x3sjasowoarfbgcmvfimaftt6twagswzczad.onion'
  );
  const [hiddenServicePort, setHiddenServicePort] = React.useState(20000);
  const [hiddenServiceDestinationPort, setHiddenServiceDestinationPort] =
    React.useState(20011);
  const [hiddenServiceKey, setHiddenServiceKey] = React.useState('');
  const [hiddenServiceOnion, setHiddenServiceOnion] = React.useState('');
  const [hasStream, setHasStream] = React.useState(false);
  const [streamConnectionTimeoutMS, setStreamConnectionTimeoutMS] =
    React.useState(15000);
  React.useEffect(() => {
    _init();
  }, []);

  const _init = async () => {
    try {
      // Init and do a few basic calls to test all is good
      console.log('App init tests');
      const socks = await client.startIfNotStarted();
      console.log('App init setting socks port to ', socksPort);
      console.log('App Sock Status', client.getDaemonStatus());
      setSocksPort(socks);
      await getOnion();
      console.log('App Sock Status post', client.getDaemonStatus());
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
  const startTcpStream = async () => {
    try {
      let target =
        'kciybn4d4vuqvobdl2kdp3r2rudqbqvsymqwg4jomzft6m6gaibaf6yd.onion:50001';
      console.log('starting');
      const conn = await client.createTcpConnection(
        { target, connectionTimeout: streamConnectionTimeoutMS },
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
            {/* Onion */}
            <TextInput
              style={{ height: 40, borderColor: 'gray', borderWidth: 1 }}
              onChangeText={setOnion}
              value={onion}
            />
            <Button onPress={getOnion} title="Get onion" />
            <Button onPress={postOnion} title="POST onion" />
            <Button onPress={startTcpStream} title="Start Tcp Stream" />
            {/* HS */}
            <TextInput
              style={{ height: 40, borderColor: 'gray', borderWidth: 1 }}
              onChangeText={(v) => setHiddenServicePort(Number(v))}
              value={hiddenServicePort.toString()}
            />
            <TextInput
              style={{ height: 40, borderColor: 'gray', borderWidth: 1 }}
              onChangeText={(v) => setHiddenServiceDestinationPort(Number(v))}
              value={hiddenServiceDestinationPort.toString()}
            />
            <TextInput
              style={{ height: 40, borderColor: 'gray', borderWidth: 1 }}
              onChangeText={(v) => setHiddenServiceKey(v)}
              value={hiddenServiceKey}
            />
            <TextInput
              style={{ height: 40, borderColor: 'gray', borderWidth: 1 }}
              onChangeText={(v) => setHiddenServiceOnion(v)}
              value={hiddenServiceOnion}
            />
            <Button
              onPress={async () => {
                const hs = await client.createHiddenService(
                  hiddenServicePort,
                  hiddenServiceDestinationPort,
                  hiddenServiceKey.length ? hiddenServiceKey.trim() : ''
                );
                setHiddenServiceKey(hs.secretKey);
                setHiddenServiceOnion(hs.onionUrl);
              }}
              title="Create HS"
            />
            <Button
              disabled={hiddenServiceOnion?.length < 1}
              onPress={async () => {
                await client.deleteHiddenService(hiddenServiceOnion);
              }}
              title="Delete Hidden Serivce"
            />
            <Button
              onPress={() => {
                client.startHttpService(
                  hiddenServiceDestinationPort,
                  (d, e) => {
                    Alert.alert(
                      `Got HTTP ${e ? 'Error' : 'Request'} `,
                      `${JSON.stringify(d)} ${JSON.stringify(e)}`
                    );
                  }
                );
              }}
              title="Start HTTP server"
            />
            {/* Streams */}
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
