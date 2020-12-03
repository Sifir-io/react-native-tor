# React Native Tor
A fully featured Tor Daemon and Onion Routing Client for your React Native iOS and Android Project!
:calling: :closed_lock_with_key: :globe_with_meridians:
## TL;DR
In your project:

```shell script
npm i react-native-tor
// for ios builds
// cd ios && pod install
```

then
```js
import Tor from "react-native-tor";

// Initialize the module
const tor = Tor();

//...

const makeTorRequest = async()=>{
    // Start the daemon and socks proxy (no need for Orbot and yes iOS supported!)
    await tor.startIfNotStarted();

    try{
       // Use built in client to make REST calls to .onion urls routed through the Sock5 proxy !
       const resp = await tor.get('http://some.onion',{'Authoriztion': 'sometoken'});
       // Note: self signed https endpoints, custom headers and multiform/json payloads supported!
       await tor.post('https://someother.onion',JSON.stringify(resp.json),{'Authoriztion': 'sometoken'},true);

    } catch(error){
      // Catch a network or server error like you normally with any other fetch library
    }
}

makeTorRequest();
```
:boom:

If you think this is awesome, please consider [contributing to Privacy and Opensource](#Contributing-to-Privacy-and-Opensource) !

## Highlights
- Embeds a fully functional Tor Daemon, with its own circuit (non exit) removing the dependency on Orbot and allowing Tor usage on IOS.
- Provides a Socks5 proxy enabled REST client to allow you to make Rest calls on Onion URLs directly from JS just as you would with Axios, Frisbee etc..!
- [WIP] Start a hidden service accessible via an Onion URL directly on your phone (in final test for upcoming 0.0.2 release)
- Provides guard functions and state management options to autostart/stop the daemon when REST calls are initiated and/or the application is backgrounded/foregrounded
- TS Typed API for sanity.

## Disclaimer
Early Beta software provided as is and accepts no responsibility for use.

---

## Why This ?
Privacy is important and should not be hard.
- Users shouldn't be expected to install 3rd party apps and setup custom VPNS to get more privacy.
- Devs shouldn't have to jump through hoops to get Onion urls routing correctly
- React native is an awesome tool and this brings awesome privacy to it.


## What This Does ?
Embeds Tor Daemon + Onion Routing into your App, removing the dependency on Orbot and allowing Tor usage on IOS

## How it Does it ?

This module uses multiple layers:
1. Pre-compiled rust sdk (sifir-rs-sdk) for the Tor daemon and Control code (Check CODE section for details)
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
## API reference
Please reference [Typescript defs and JSDoc](./src/index.tsx) for details.

Documentation is a WIP, Contribute!


```ts
const tor = Tor({ ...params });
```
defined as :
```ts
/**
 * Tor module factory function
 * @param stopDaemonOnBackground
 * @default true
 * When set to true will shutdown the Tor daemon when the application is backgrounded preventing pre-emitive shutdowns by the OS
 * @param startDaemonOnActive
 * @default false
 * When set to true will automatically start/restart the Tor daemon when the application is bought back to the foreground (from the background)
 * @param os The OS the module is running on (Set automatically and is provided as an injectable for testing purposes)
 * @default The os the module is running on.
 */
declare const _default: ({ stopDaemonOnBackground, startDaemonOnActive, os, }?: {
    stopDaemonOnBackground?: boolean | undefined;
    startDaemonOnActive?: boolean | undefined;
    os?: "ios" | "android" | "windows" | "macos" | "web" | undefined;
}) => TorType;
```
This returns an instance of TorType module which gives control over the Tor daemon and access to the Socks5 enabled client:
```ts
declare type TorType = {
    /**
     * Send a GET request routed through the SOCKS proxy on the native side
     * Starts the Tor Daemon automatically if not already started
     * @param url
     * @param headers
     * @param trustSSL @default true When true accepts *any* SSL cert for the onion URL
     */
    get(url: string, headers?: RequestHeaders, trustSSL?: boolean): Promise<ProcessedRequestResponse>;
    /**
     * Send a POST request routed through the SOCKS proxy on the native side
     * Starts the Tor Daemon automatically if not already started
     * @param url
     * @param body JSON encoded (JSON.stringify) string of the payload to send * *Note*: Client will treat the body as a JSON string and set request header accordingly
     * The client also supports sending the payload as `application/x-www-form-urlencoded` by setting the `Content-Type` header to `application/x-www-form-urlencoded` in the header param
     * @param headers
     * @param trustSSL @default true When true accepts *any* SSL cert for the onion URL
     */
    post(url: string, body: RequestBody[RequestMethod.POST], headers?: RequestHeaders, trustSSL?: boolean): Promise<ProcessedRequestResponse>;
    /**
     * Send a DELETE request routed through the SOCKS proxy on the native side
     * Starts the Tor Daemon automatically if not already started
     * @param url
     * @param headers
     * @param trustSSL @default true When true accepts *any* SSL cert for the onion URL
     */
    delete(url: string, body?: RequestBody[RequestMethod.DELETE], headers?: RequestHeaders, trustSSL?: boolean): Promise<ProcessedRequestResponse>;
    /** Starts the Tor Daemon if not started and returns a promise that fullfills with the socks port number when boostraping is complete.
     * If the function was previously called it will return the promise without attempting to start the daemon again.
     * Useful when used as a guard in your transport or action layer
     */
    startIfNotStarted(): Promise<SocksPortNumber>;
    /**
     * Stops a running Tor Daemon
     */
    stopIfRunning(): Promise<void>;
    /**
     * Returns the current status of the Daemon
     * Some :
     * NOTINIT - Not initialized or run (call startIfNotStarted to the startDaemon)
     * STARTING - Daemon is starting and bootsraping
     * DONE - Daemon has completed boostraing and socks proxy is ready to be used to route traffic.
     * <other> - A status returned directly by the Daemon that can indicate a transient state or error.
     */
    getDaemonStatus(): Promise<string>;
    /**
     * Accessor the Native request function
     * Should not be used unless you know what you are doing.
     */
    request: NativeTor['request'];
};
```

You can also check the provided [Example Application](example/src/App.tsx) for a quick reference.

---
## Acknowledgments

- [Torproject](https://www.torproject.org) For everything they do.
- [@afilini](https://github.com/afilini) for his amazing work on libtor and constant support!

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

## Development and Building from Scratch

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

- Better API Docs
- Event emitter from Rust to Native on Boostrap status
- Enable secret service API
    - Start new secret service on phone.
    - Restore secret service from key.

### Backlog

- Search for available ports for socks proxy for iOS
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
