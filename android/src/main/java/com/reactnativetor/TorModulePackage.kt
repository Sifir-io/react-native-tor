package com.reactnativetor

import android.util.Log
import com.facebook.react.bridge.JSIModulePackage
import com.facebook.react.bridge.ReactApplicationContext
import com.facebook.react.bridge.JavaScriptContextHolder
import com.facebook.react.bridge.JSIModuleSpec
import com.reactnativetor.TorModule

class TorModulePackage : JSIModulePackage {
  init {
    System.loadLibrary("react-native-tor-cpp");
  }

//  private external fun nativeInstall(jsi: Long);

  override fun getJSIModules(
    reactApplicationContext: ReactApplicationContext,
    jsContext: JavaScriptContextHolder
  ): List<JSIModuleSpec<*>> {
    val tModule =  TorModule(reactContext = reactApplicationContext)
    tModule?.installLib(reactContext = jsContext);
    return emptyList()
  }
}
