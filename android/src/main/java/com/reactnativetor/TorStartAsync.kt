package com.reactnativetor

import android.os.AsyncTask
import android.util.Log
import com.facebook.react.bridge.*
import java.io.IOException
import com.sifir.tor.OwnedTorService
import com.sifir.tor.TorServiceParam

class StartParam(
  val socksPort: Int,
  var path: String
);

class TorBridgeStartAsync constructor(
  private val param: StartParam,
  private val onSuccess: (service: OwnedTorService) -> Unit,
  private val onError: (e: Throwable) -> Unit
) {
  fun run() {
    try {
      val ownedTor = OwnedTorService(TorServiceParam(param.path, param.socksPort));
      onSuccess(ownedTor);
    } catch (e: Error) {
      Log.d("TorBridge:StartAsync", "error onPostExecute$e")
      onError(e as Throwable);
    }
  }
}
