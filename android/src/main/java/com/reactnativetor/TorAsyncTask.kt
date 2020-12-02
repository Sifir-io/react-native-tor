package com.reactnativetor

import android.os.AsyncTask
import android.util.Base64
import android.util.Log
import com.facebook.react.bridge.*
import okhttp3.*
import org.json.JSONObject
import java.io.IOException

class TaskParam(
  val method: String,
  val url: String,
  val json: String?,
  val headers: HashMap<String, Any>?
);


sealed class RequestResult {
  data class Success(val result: WritableMap) : RequestResult()
  data class Error(val message: String, val result: String?, val error: Throwable?) :
    RequestResult()
}

class TorBridgeAsyncTask(protected var mPromise: Promise?, protected var client: OkHttpClient) :
  AsyncTask<TaskParam, String?, RequestResult?>() {
  override fun onPostExecute(result: RequestResult?) {
    when (result) {
      is RequestResult.Error -> {
        if (result.error !== null) {
          mPromise!!.reject(result.message, result.error);
        } else if (result.result != null) {
          mPromise!!.reject(result.message, Throwable(result.message + ": " + result.result));
        }
      }
      is RequestResult.Success -> mPromise!!.resolve(result.result)
      else -> mPromise!!.reject("Unable to processing Request result")
    }
    mPromise = null
  }


  fun run(param: TaskParam): RequestResult {

    val request = when (param.method.toUpperCase()) {
      "POST" -> {
        // Check Content-Type headers provided
        // Currently only support application/x-www-form-urlencoded
        // If not provided defaults to application/json
        // TODO Expand supported content formats ?
        val body = when (param.headers?.get("Content-Type") ?: "application/json") {
          "application/x-www-form-urlencoded" -> FormBody.create(
            MediaType.get("application/x-www-form-urlencoded; charset=utf-8"),
            param.json!!
          )
          else -> RequestBody.create(MediaType.get("application/json; charset=utf-8"), param.json!!)
        }
        Request.Builder().url(param.url)
          .post(body)
      }
      "GET" -> Request.Builder().url(param.url)
      // TODO add body support for delete
      "DELETE" -> Request.Builder().url(param.url).delete()
      // TODO PUT
      else -> throw IOException("Invalid method $param.method")
    }

    if (!param.headers.isNullOrEmpty()) {
      param.headers.forEach { (key, value) -> request.addHeader(key, value.toString()); }
    }


    client.newCall(request.build()).execute().use { response ->
      val resp = Arguments.createMap()
      val headersMap = Arguments.createMap()

      response.headers().toMultimap().map {
        headersMap.putArray(it.key.toString(), Arguments.fromList(it.value))
      }
      resp.putMap("headers", headersMap)

      val respCode = response.code()
      resp.putInt("respCode", respCode)

      val contentType = response.header("content-type").toString();
      val body = response.body()?.bytes()
      if (contentType is String) {
        resp.putString("mimeType", contentType)
        if (contentType.startsWith("application/json") || contentType.startsWith("application/javascript")) {
          // Android JSONObjects -> WritableMaps are a mess
          // So just send the string back to JS to parse
          resp.putString("json", body?.toString(Charsets.UTF_8))
        }
      }
      resp.putString("b64Data", Base64.encodeToString(body, Base64.DEFAULT))

      return if (response.code() > 299) {
        RequestResult.Error(
          "Request Response Code ($respCode)",
          body?.toString(Charsets.UTF_8),
          null
        );
      } else {
        RequestResult.Success(resp);
      }
    }
  }

  override fun doInBackground(vararg params: TaskParam?): RequestResult {
    return try {
      run(params[0]!!)
    } catch (e: Exception) {
      Log.d("TorBridge", "error doInBackground$e")
      return RequestResult.Error("Error processing Request", null, e)
    }
  }

}
