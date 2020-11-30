# react-native-tor

Tor for React Native

## Installation

```sh
npm install react-native-tor
```

## Usage

```js
import Tor from "react-native-tor";

// ...

const result = await Tor.multiply(3, 7);
```

## Contributing

See the [contributing guide](CONTRIBUTING.md) to learn how to contribute to the repository and the development workflow.

## TODO
- Accept socks port param
	- Accept socks port parameter and check if port is avalible
- Build on Request capability
  - PUT calls
  - DELETE 
    - Add body support
  - Websockets
  - Streaming ?
- Maybe make it as a NetworkExtension and create VPN for app so we can use started REST Libraries ?
## License

MIT
