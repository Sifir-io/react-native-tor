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

class TorBridgeStartAsync(onSuccess: (service: OwnedTorService) -> Unit, onError: (e: Throwable) -> Unit) : AsyncTask<StartParam, String?, OwnedTorService?>() {
  protected var error: Exception? = null
  protected var onError = onError;
  protected var onSuccess = onSuccess;

  override fun onPostExecute(result: OwnedTorService?) {
    if (error != null || result == null) {
      Log.d("TorBridge:StartAsync", "error onPostExecute" + error.toString())
      onError(error as Throwable);
    } else {
      onSuccess(result);
    }
    error = null
  }

  @Throws(IOException::class)
  fun run(param: StartParam): OwnedTorService {
    return OwnedTorService(TorServiceParam(param.path, param.socksPort));
  }

  override fun doInBackground(vararg params: StartParam?): OwnedTorService? {
    return try {
      run(params[0]!!)
    } catch (e: Exception) {
      Log.d("TorBridge", "error doInBackground$e")
      error = e
      return null;
    }
  }

}
