package com.reactnativetor

import android.os.AsyncTask
import android.util.Base64
import android.util.Log
import com.facebook.react.bridge.*
import okhttp3.MediaType
import okhttp3.OkHttpClient
import okhttp3.Request
import okhttp3.RequestBody
import org.json.JSONObject
import java.io.IOException

class TaskParam(
  val method: String,
  val url: String,
  val json: String?,
  val headers: HashMap<String, Any>?
);

class TorBridgeAsyncTask(protected var mPromise: Promise?, protected var client: OkHttpClient) : AsyncTask<TaskParam, String?, WritableMap?>() {
  protected var error: Exception? = null
  override fun onPostExecute(result: WritableMap?) {
    if (error != null) {
      Log.d("TorBridge", "error onPostExecute" + error.toString())
      mPromise!!.reject(error)
    } else {
      mPromise!!.resolve(result)
    }
    mPromise = null
    error = null
  }

  @Throws(IOException::class)
  fun run(param: TaskParam): WritableMap {
    val request: Request.Builder
    request = when (param.method.toUpperCase()) {
      "POST" -> {
        // FIXME body should not be empty for post ??
        val body = RequestBody.create(JSON, param.json!!)
        Request.Builder()
          .url(param.url)
          .post(body)
      }
      "GET" -> Request.Builder()
        .url(param.url)
      "DELETE" -> Request.Builder().url(param.url).delete()
      else -> throw IOException("Invalid method $param.method")
    }
    if (!param.headers.isNullOrEmpty()) {
      param.headers.forEach { (key, value) -> request.addHeader(key, value.toString()); }
    }

    client.newCall(request.build()).execute().use { response ->
      val resp = Arguments.createMap()
      val headers = response.headers().toMultimap()
      val headersMap = Arguments.createMap()
      for ((key, value) in headers.entries) {
        try {
          // FIXME headers seem to load on duckgo but not on json.
          // is json header https://api.jsonapi.co/rest/v1/user/list not a List  ?
          if (value is List<*>) {
            headersMap.putArray(key.toString(), Arguments.fromList(headers[key]))
          } else {
            Log.d("TOR","Header value is not list");
          }
        } catch (e: Throwable) {
        }
      }
      resp.putMap("headers", headersMap)
      val contentType = response.header("content-type").toString();
      val body = response.body()?.bytes()
      if (contentType is String) {
        resp.putString("mimeType", contentType)
        if (contentType.startsWith("application/json") || contentType.startsWith("application/javascript")) {
          resp.putString("json", JSONObject(body?.toString(Charsets.UTF_8)).toString())
        }
      }
      resp.putString("b64Data", Base64.encodeToString(body, Base64.DEFAULT))
      // FIXME our return type
      return resp
    }
  }

  companion object {
    protected val JSON = MediaType.get("application/json; charset=utf-8")
  }

  override fun doInBackground(vararg params: TaskParam?): WritableMap? {
    return try {
      run(params[0]!!)
    } catch (e: Exception) {
      Log.d("TorBridge", "error doInBackground$e")
      error = e
      val resp = Arguments.createMap()
      resp.putString("error", e.toString())
      resp
    }
  }

}
