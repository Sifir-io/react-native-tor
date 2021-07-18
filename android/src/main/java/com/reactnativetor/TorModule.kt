package com.reactnativetor

import android.content.Context
import android.util.Log
import com.facebook.react.bridge.*
import com.sifir.tor.DataObserver
import com.sifir.tor.OwnedTorService
import com.sifir.tor.TcpSocksStream
import com.sifir.tor.HiddenServiceHandler
import okhttp3.OkHttpClient
import java.io.IOException
import java.net.InetSocketAddress
import java.net.ServerSocket
import java.net.Proxy;
import java.security.cert.X509Certificate
import javax.net.ssl.SSLContext
import javax.net.ssl.TrustManager
import javax.net.ssl.X509TrustManager
import com.facebook.react.modules.core.DeviceEventManagerModule.RCTDeviceEventEmitter
import org.json.JSONObject
import java.util.UUID;
import java.util.concurrent.*


/**
 * Wraps DataObserver interface into event emitter
 * Sent across FFI and will emit on data based on target-data or target-error topic
 */
class DataObserverEmitter(
  private val connId: String,
  private val reactContext: ReactApplicationContext,
  private val streams: HashMap<String, TcpSocksStream>
) : DataObserver {
  override fun onData(p0: String?) {
    reactContext
      .getJSModule(RCTDeviceEventEmitter::class.java)
      .emit("$connId-data", p0)
  }

  override fun onError(p0: String?) {
    // Shutdown and remove it from map on EOF
    // To prevent invalid memory pointer
    // TODO Change this when we implement streaming streams.
    if (p0 == "EOF") {
      try {
        Log.d("TorBridge", "DataObserver: EOF detected from '$connId', deleting stream..")
        streams.remove(connId)?.delete();
      } catch (e: Exception) {
        Log.d("TorBridge", "DataObserver:Error deleting stream for '$connId': $e")
      }
    }
    reactContext
      .getJSModule(RCTDeviceEventEmitter::class.java)
      .emit("$connId-error", p0)
  }
}

class HttpHandlerObserverEmitter(
  private val handlerId: String,
  private val reactContext: ReactApplicationContext,
  private val serviceHandlers: HashMap<String, HiddenServiceHandler>
) : DataObserver {
  override fun onData(p0: String?) {
    reactContext
      .getJSModule(RCTDeviceEventEmitter::class.java)
      .emit("$handlerId-data", p0)
  }

  override fun onError(p0: String?) {
    reactContext
      .getJSModule(RCTDeviceEventEmitter::class.java)
      .emit("$handlerId-error", p0)
  }
}

class TorModule(reactContext: ReactApplicationContext) : ReactContextBaseJavaModule(reactContext) {
  private var _client: OkHttpClient? = null;
  private var service: OwnedTorService? = null;
  private var proxy: Proxy? = null;
  private var _starting: Boolean = false;
  private var _streams: HashMap<String, TcpSocksStream> = HashMap();
  private val _serviceHandlers: HashMap<String, HiddenServiceHandler> = HashMap();

  //  private val executorService: ExecutorService = Executors.newFixedThreadPool(4)
  private val executorService: ThreadPoolExecutor =
    ThreadPoolExecutor(1, 1, 0L, TimeUnit.MILLISECONDS, LinkedBlockingQueue<Runnable>(50));


  /**
   * Gets a client that accepts all SSL certs
   * TODO Not sure how i feel about this but seems to be the only way
   * to accept self signed certs for onion urls
   * if anyone knows of a better way please PR this.
   */
  private fun getUnsafeOkHttpClient(): OkHttpClient.Builder {
    // Create a trust manager that does not validate certificate chains
    val trustAllCerts = arrayOf<TrustManager>(object : X509TrustManager {
      override fun checkClientTrusted(chain: Array<out X509Certificate>?, authType: String?) {
      }

      override fun checkServerTrusted(chain: Array<out X509Certificate>?, authType: String?) {
      }

      override fun getAcceptedIssuers() = arrayOf<X509Certificate>()
    })

    // Install the all-trusting trust manager
    val sslContext = SSLContext.getInstance("SSL")
    sslContext.init(null, trustAllCerts, java.security.SecureRandom())
    // Create an ssl socket factory with our all-trusting manager
    val sslSocketFactory = sslContext.socketFactory

    return OkHttpClient.Builder()
      .sslSocketFactory(sslSocketFactory, trustAllCerts[0] as X509TrustManager)
      .hostnameVerifier { _, _ -> true }
  }

  override fun getName(): String {
    return "TorBridge"
  }

  private fun findFreePort(): Int {
    var socket: ServerSocket? = null
    try {
      socket = ServerSocket(0)
      socket.reuseAddress = true
      val port = socket.localPort
      try {
        socket.close()
      } catch (e: IOException) {
        // Ignore IOException on close()
      }
      return port
    } catch (e: IOException) {
    } finally {
      if (socket != null) {
        try {
          socket.close()
        } catch (e: IOException) {
        }
      }
    }
    throw Throwable("Could not find a free TCP/IP port for Socks Proxy")
  }

  @ReactMethod
  fun request(
    url: String,
    method: String,
    jsonBody: String,
    headers: ReadableMap,
    // FIXME move this to startDeamon call
    trustAllSSl: Boolean,
    promise: Promise
  ) {
    if (service == null) {
      promise.reject(Throwable("Service Not Initialized!, Call startDaemon first"));
      return;
    }

//    if(_client !is OkHttpClient){
//       _client = (if (trustAllSSl) getUnsafeOkHttpClient() else OkHttpClient().newBuilder())
//        .proxy(proxy)
//        .connectTimeout(10, TimeUnit.SECONDS)
//        .writeTimeout(10, TimeUnit.SECONDS)
//        .readTimeout(10, TimeUnit.SECONDS)
//        .build()
//    }

    if(_client !is OkHttpClient){
      promise.reject(Throwable("Request http client not Initialized!, Call startDaemon first"));
      return;
    }

    val param = TaskParam(method, url, jsonBody, headers.toHashMap())
    executorService.execute {
      try {
        val task = TorBridgeRequest(promise, _client!!, param);
        task.run()
      } catch (e: Exception) {
        Log.d("TorBridge", "error on request: $e")
        promise.reject(e)
      }
    }
  }


  @ReactMethod
  fun startDaemon(timeoutMs: Double, promise: Promise) {
    if (service != null) {
      promise.reject(Throwable("Service already running, call stopDaemon first"))
      return;
    }
    if (this._starting) {
      promise.reject(Throwable("Service already starting"))
      return;
    }
    _starting = true;
    executorService.execute {
      val socksPort = findFreePort();
      val path = this.reactApplicationContext.cacheDir.toString();
      val param = StartParam(socksPort, path, timeoutMs.toLong())
      try {
        TorBridgeStartAsync(param, {
          service = it
          proxy = Proxy(Proxy.Type.SOCKS, InetSocketAddress("0.0.0.0", socksPort))
          _starting = false;

          _client = getUnsafeOkHttpClient()
            .proxy(proxy)
            .connectTimeout(10, TimeUnit.SECONDS)
            .writeTimeout(10, TimeUnit.SECONDS)
            .readTimeout(10, TimeUnit.SECONDS)
            .build();

          promise.resolve(socksPort);
        }, {
          _starting = false;
          promise.reject("StartDaemon Error", "Error starting Tor Daemon", it);
        }).run();

      } catch (e: Exception) {
        Log.d("TorBridge", "error on startDaemon: $e")
        promise.reject(e)
      }
    }
  }

  @ReactMethod
  fun getDaemonStatus(promise: Promise) {
    if (_starting) {
      promise.resolve("STARTING");
      return;
    }
    if (service == null) {
      promise.resolve("NOTINIT");
      return;
    }

    val status = service?.get_status();
    promise.resolve(status);
  }

  @ReactMethod
  fun stopDaemon(promise: Promise) {
    try {
      service?.shutdown();
      service = null
      proxy = null;
      promise.resolve(true);
    } catch (e: Exception) {
      Log.d("TorBridge", "error on stopDaemon$e")
      promise.reject(e)
    }
  }

  @ReactMethod
  fun startTcpConn(target: String, timeoutMs: Double, promise: Promise) {
    executorService.execute {
      try {
        if (service == null) {
          throw Exception("Tor service not running, call startDaemon first")
        }
        TcpStreamStart(target, "0.0.0.0:${service?.socksPort}", timeoutMs.toLong(), {
          // Assign UUID to connection to manage it
          val uuid = UUID.randomUUID();
          val connId = uuid.toString();
          it.on_data(DataObserverEmitter(connId, this.reactApplicationContext, _streams));
          _streams.set(connId, it);
          Log.d("TorBridge", "Connection to $target created and assigned connection Id $connId");
          promise.resolve(connId);
        }, {
          Log.d("TorBridge", "error on startTcpConn: $it")
          promise.reject(it)
        }).run();
      } catch (e: Exception) {
        Log.d("TorBridge", "error on startTcpConn: $e")
        promise.reject(e)
      }
    }
  }

  @ReactMethod
  fun sendTcpConnMsg(connId: String, msg: String, timeoutSec: Double, promise: Promise) {
    try {
      if (service == null) {
        throw Throwable("Tor Service not running, call startDaemon first")
      }
      var stream = _streams[connId]
        ?: throw Throwable("Stream for connectionId $connId is not initialized, call startTcpConn first");
      stream.send_data(msg, timeoutSec.toLong());
      promise.resolve(true);
    } catch (e: Exception) {
      Log.d("TorBridge", "error on sendTcpConnMsg on connection Id $connId : $e")
      promise.reject(e)
    } catch (e: Throwable) {
      Log.d("TorBridge", "error on sendTcpConnMsg on connection ID $connId : $e")
      promise.reject(e)
    }
  }

  @ReactMethod
  fun stopTcpConn(connId: String, promise: Promise) {
    try {
      _streams.remove(connId)?.delete();
      promise.resolve(true);
    } catch (e: Exception) {
      Log.d("TorBridge", "error on stopTcpConn for connection Id $connId : $e")
      promise.reject(e)
    }
  }

  @ReactMethod
  fun createHiddenService(
    hiddenServicePort: Int,
    destinationPort: Int,
    secretKey: String,
    promise: Promise
  ) {
    try {
      if (service == null) {
        promise.reject(Throwable("Service Not Initialized!, Call startDaemon first"));
        return;
      }
      val serviceDetails =
        service!!.create_hidden_service(destinationPort,hiddenServicePort, secretKey);

      val result = HashMap<Any?, Any?>();

      result.set("onionUrl", serviceDetails.get_onion_url());
      result.set("secretKey", serviceDetails.get_secret_b64());

      promise.resolve(JSONObject(result).toString());

    } catch (e: Exception) {
      Log.d("TorBridge", "error on createHiddenService : $e")
      promise.reject(e)
    }
  }
  @ReactMethod
  fun deleteHiddenService(
    onion: String,
    promise: Promise
  ) {
    try {
      if (service == null) {
        promise.reject(Throwable("Service Not Initialized!, Call startDaemon first"));
        return;
      }
      service!!.delete_hidden_service(onion);

      promise.resolve(true);

    } catch (e: Exception) {
      Log.d("TorBridge", "error on deleteHiddenService : $e")
      promise.reject(e)
    }
  }

  @ReactMethod
  fun startHttpHiddenserviceHandler(port: Int, promise: Promise) {
    try {
      val uuid = UUID.randomUUID();
      val serviceId = uuid.toString();
      val serviceHandler = HiddenServiceHandler(
        port,
        HttpHandlerObserverEmitter(serviceId, this.reactApplicationContext, _serviceHandlers)
      );
      _serviceHandlers.set(serviceId, serviceHandler);
      promise.resolve(serviceId);
    } catch (e: Exception) {
      Log.d("TorBridge", "error on startHttpHiddenserviceHandler : $e")
      promise.reject(e)
    }

    @ReactMethod
    fun stopHttpHiddenserviceHandler(id: String, promise: Promise) {
      try {
        _serviceHandlers.remove(id)?.delete();
        promise.resolve(true);
      } catch (e: Exception) {
        Log.d("TorBridge", "error on stopHttpHiddenserviceHandler for connection Id $id : $e")
        promise.reject(e)
      }

    }

  }
}
