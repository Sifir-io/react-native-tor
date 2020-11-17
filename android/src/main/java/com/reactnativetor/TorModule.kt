package com.reactnativetor

import com.facebook.react.bridge.Promise
import com.facebook.react.bridge.ReactApplicationContext
import com.facebook.react.bridge.ReactContextBaseJavaModule
import com.facebook.react.bridge.ReactMethod
import com.sifir.tor.OwnedTorService
import com.sifir.tor.TorServiceParam
import java.io.IOException
import java.net.ServerSocket


class TorModule(reactContext: ReactApplicationContext) : ReactContextBaseJavaModule(reactContext) {

    lateinit var service:OwnedTorService;

    override fun getName(): String {
        return "Tor"
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
    throw IllegalStateException("Could not find a free TCP/IP port to start embedded Jetty HTTP Server on")
  }


    @ReactMethod
    fun startDaemon(promise: Promise) {
      if(::service.isInitialized) {
          service.shutdown()
      }
      val socksPort = findFreePort();
      val path = System.getProperty("user.dir")
      service = OwnedTorService(TorServiceParam(path, socksPort))
      promise.resolve(socksPort);
    }

    @ReactMethod
    fun stopDaemon(promise: Promise) {
      if(::service.isInitialized){
          service.shutdown();
      }
      promise.resolve(true);
    }
}
