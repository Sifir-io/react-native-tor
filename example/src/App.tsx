import * as React from 'react';
import { StyleSheet, View, Text, Button } from 'react-native';
import Tor from 'react-native-tor';

export default function App() {
  const [socksPort, setSocksPort] = React.useState<number | undefined>();

  const startTor = async () => {
    const port = await Tor.startDaemon();
    console.log('Tor started socks port', port);
    setSocksPort(port);
  };

  const stopTor = async () => {
    await Tor.stopDaemon();
    setSocksPort(undefined);
    console.log('Tor stopped');
  };
  return (
    <View style={styles.container}>
      <View>
        <Button onPress={startTor} title="Start Tor">
          <Text>Start Tor</Text>
        </Button>

        <Button onPress={stopTor} title="Stop Tor" disabled={!!socksPort}>
          <Text>Stop Tor</Text>
        </Button>
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
