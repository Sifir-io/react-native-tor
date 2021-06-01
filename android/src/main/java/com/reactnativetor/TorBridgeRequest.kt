package com.reactnativetor

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

class TorBridgeRequest constructor(
  protected var mPromise: Promise?,
  protected var client: OkHttpClient,
  protected val param: TaskParam
) {

  protected fun onPostExecute(result: RequestResult?){
    when (result) {
      is RequestResult.Error -> {
        if (result.error !== null) {
          mPromise!!.reject(result.message, result.error);
        } else if (result.result != null) {
          mPromise!!.reject(result.message, Throwable(result.message + ": " + result.result));
        }
      }
      is RequestResult.Success -> mPromise!!.resolve(result.result)
      else -> mPromise!!.reject("Unable to process RequestResult","RequestResult Exhaustive Clause")
    }
    mPromise = null
  }


  public fun run() {
    val request = when (param.method.toUpperCase()) {
      "POST" -> {
        // Check Content-Type headers provided
        // Currently only supports application/x-www-form-urlencoded
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

      if (response.code() > 299) {
        onPostExecute(
          RequestResult.Error(
            "Request Response Code ($respCode)",
            body?.toString(Charsets.UTF_8),
            null
          )
        );
      } else {
        onPostExecute(RequestResult.Success(resp));
      }
    }
  }


}
