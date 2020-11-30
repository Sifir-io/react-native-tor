package com.reactnativetor

import android.os.AsyncTask
import android.util.Log
import com.facebook.react.bridge.*
import com.sifir.tor.OwnedTorService
import com.sifir.tor.TorServiceParam
import okhttp3.OkHttpClient
import java.io.IOException
import java.net.InetSocketAddress
import java.net.ServerSocket
import java.util.concurrent.TimeUnit
import java.net.Proxy;


class TorModule(reactContext: ReactApplicationContext) : ReactContextBaseJavaModule(reactContext) {
  private var service: OwnedTorService? = null;
  private var proxy: Proxy? = null;
  private var client: OkHttpClient? = null;
  private var _starting: Boolean = false;

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
  fun request(url: String, method: String, jsonBody: String, headers: ReadableMap, promise: Promise) {
    if (service == null) {
      promise.reject(Throwable("Service Not Initialized!, Call startDaemon first"));
    }
    try {
      val param = TaskParam(method, url, jsonBody, headers.toHashMap())
      TorBridgeAsyncTask(promise, client!!).executeOnExecutor(
        AsyncTask.THREAD_POOL_EXECUTOR, param
      )
    } catch (e: Exception) {
      Log.d("TorBridge", "error on sendRequest$e")
      promise.reject(e)
    }
  }


  @ReactMethod
  fun startDaemon(promise: Promise) {
    if (service != null) {
      promise.reject(Throwable("Service already running, call stopDaemon first"))
    }
    if (this._starting) {
      promise.reject(Throwable("Service already starting"))
    }
    _starting = true;
    try {
      val socksPort = findFreePort();
      val path = this.reactApplicationContext.cacheDir.toString();
      val param = StartParam(socksPort, path)
      TorBridgeStartAsync({
        service = it
        proxy = Proxy(Proxy.Type.SOCKS, InetSocketAddress("0.0.0.0", socksPort))
        client = OkHttpClient()
          .newBuilder()
          .proxy(proxy)
          .connectTimeout(7, TimeUnit.SECONDS)
          .writeTimeout(7, TimeUnit.SECONDS)
          .readTimeout(7, TimeUnit.SECONDS)
          .build()
        _starting = false;
        promise.resolve(socksPort);
      }, {
        _starting = false;
        promise.reject(it);
      }).executeOnExecutor(
        AsyncTask.THREAD_POOL_EXECUTOR, param
      )
    } catch (e: Exception) {
      Log.d("TorBridge", "error on sendRequest$e")
      promise.reject(e)
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
      client = null;
      promise.resolve(true);
    } catch (e: Exception) {
      Log.d("TorBridge", "error on stopDaemon$e")
      promise.reject(e)
    }
  }
}
