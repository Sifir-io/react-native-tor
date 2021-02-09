package com.reactnativetor

import android.os.AsyncTask
import android.util.Log
import com.facebook.react.bridge.*
import java.io.IOException
import com.sifir.tor.OwnedTorService
import com.sifir.tor.TcpSocksStream
import com.sifir.tor.TorServiceParam

class TcpStreamStart constructor(
  private val target: String,
  private val proxy:String,
  private val timeoutMs:Long,
  private val onSuccess: (stream: TcpSocksStream) -> Unit,
  private val onError: (e: Throwable) -> Unit
) {
  fun run() {
    try {
      val stream = TcpSocksStream(target,proxy,timeoutMs);
      onSuccess(stream);
    } catch (e: Error) {
      Log.d("TorBridge:TcpStream", "error $e")
      onError(e as Throwable);
    }
  }
}
