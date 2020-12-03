# React Native Tor
A fully featured Tor Daemon and Onion Routing Client for your React Native iOS and Android Project!
:calling: :closed_lock_with_key: :globe_with_meridians: :boom:
## TL;DR
```js
import Tor from "react-native-tor";
// init client with default options
const tor = Tor();
// start the daemon and socks port (no need for Orbot and yes iOS supported!)
await tor.startIfNotStarted();
try{
   // use proxified built in client to make REST calls
   // routed through the sock5 proxy !
   // note: self signed https endpoints and custom headers supported!
   const resp = tor.get('https://some.onion',{'Authoriztion': 'sometoken'},true);
   tor.post('https://someother.onion',JSON.stringify(body),{'Authoriztion': 'sometoken'},true);
} catch(error){
  //...
}
```

If you think this is awesome, please consider [contributing to Privacy and Opensource](#Contributing-to-Privacy-and-Opensource) !

## Highlights
- Embeds Tor Daemon + Onion Routing removing the dependency on Orbot and allowing Tor usage on IOS.
- Provides a Socks5 proxy enabled native client to allow you to make Rest calls on Onion URLs directly from JS!
- Start a hidden service accessible via an Onion URL directly on your phone (in final test for upcoming 0.0.2 release)
- Provides guard functions and state management options to auto start/stop the daemon when calls are made the application is backgrounded/foregrounded
- TS Typed API for sanity.

## Disclaimer
Early Beta software provided as is and accepts no responsibility for use.

---

## Why This ?
Privacy is important and should not be hard.

We provided a Daemon + A proxified native side client to make it all easy for devs.

## What This Does ?
Embeds Tor Daemon + Onion Routing into Zeus, removing the dependency on Orbot and allowing Tor usage on IOS
Updates backend APIs to use Tor when enabled

## How it Does it ?

This module uses multiple layers:
1. Pre-compiled rust sdk for the Tor daemon (Check CODE section for details)
2. Swift/Kotlin wrappers to access and control the daemon and provide native proxified clients.
3. TS/JS Native bridges to allow access from your components.


### Requirements
- ios > 10.0
- Android minSdk > 26

### Installation

1. Install package
```sh
npm install react-native-tor
```
or
```shell script
yarn add react-native-tor
```
2. Link libs
- Android: there's nothing to do.
- iOS:
```
cd ios/
pod install
```

### Usage Example

```js
import Tor from "react-native-tor";

// Intialize the module with some settings
const tor = Tor({
  stopDaemonOnBackground :true, // Auto shut down daemon when app in background
});
// Start and boostrap the Tor daemon returning the socksProxy port that can be
// used to direct requests to.
const socksProxy = await tor.startIfNotStarted();


try{
    // Use module built in proxied client to make a get request
    const resp = tor.get('');
} catch(error){
    // Client throws on Network errors and Response codes > 299
}
```

You can also check the provided [Example Application](example/src/App.tsx) for a quick reference.

## Sample Applications
- [Sifir Wallet](https://github.com/Sifir-io/sifir-mobile-wallet) Privacy and UX focused Bitcoin based Super App.

---

## Contributing to Privacy and Opensource

This module is the product of love and dedication for principles I believe in.
If you find this module helpful or useful please do consider support it by contributing in the best way that suits you, here are some ideas:

### Fund Privacy and Open Source !

`Funding = More time to focus on what matters.`

If you found this useful please help fund development for this and other privacy focused projects,

- Send some Bitcoin/SATS:
bc1q8442hgk32h2x5undjlnu56q2ewnmnpys30zstj

- Fund Tor projects and exit nodes
https://blog.torproject.org/support-tor-network-donate-exit-node-providers

- Librepay:
https://liberapay.com/gabidi/

### Coding
- Check the TODO list below for some ideas on items flagged as valuable for users of Tor.
- Check Development section below for instructions
-
See the [contributing guide](CONTRIBUTING.md) to learn how to contribute to the repository and the development workflow.

## Developement and Building from Scratch

### Building for Native projects

There are 3 layers of libraries this project relies on:
1. The [Sifir Rust SDK](https://github.com/Sifir-io/sifir-rs-sdk/)
providing lower level control of the Tor daemon and where building the iOS and Android libraries is done.
2. The [iOS](./ios) and [Android](./android) wrapper Code to bridge and provide a socks client.
This is where the mobile native code interacts with the prebuilt Rust libraries with step1 via FFI and exposes that functionality
to React. It is also where the proxied client forwards requests recieved from React.
3. The [Typescript](./src) module. This is the module React interacts with.

Depending on the level of modifications needed you will need to work with 1 or all 3 layers.

###  Building Android AAR and adding to Android Project

https://github.com/Sifir-io/sifir-rs-sdk/
Use the helper `scripts/` for both the [Android Tor](https://github.com/Sifir-io/sifir-rs-sdk/tree/main/sifir-android/) and [iOS Tor](https://github.com/Sifir-io/sifir-rs-sdk/tree/main/sifir-ios/) modules to compile them.

### Adding compiled libs to this repo
1. Copy the compiled Android AAR to the folder `./android/libs/`
2. Copy the compiled Universal iOS dylib to the folder `./ios/library/universal/`
     a. Modify [Pod spec file](./react-native-tor.podspec) if needed
3. Bootstrap and run the example app to verify all is good
     1. `yarn bootstrap`
     2. `cd example`
     3. in one terminal `yarn start`
     4. in another terminal `yarn ios` or `yarn android`

### Modify Native code

### Modify Typescript Code

## Share and Tell your friends
Know someone who want to add a bit more privacy to their Application / Product ? Help them out and tell them about this repo!

## TODOs

### WIP
- Enable secret service API
    - Start new secret service on phone.
    - Restore secret service from key.

### Backlog
- Search for available for socks proxy for iOS
- Return a Context API for each component building.
- Build on Request capability
  - PUT calls
  - DELETE
    - Add body support
  - Websockets
  - Streaming ?
  - Maybe make it as a NetworkExtension and create VPN for app so we can use started REST Libraries ?

## License

MIT
