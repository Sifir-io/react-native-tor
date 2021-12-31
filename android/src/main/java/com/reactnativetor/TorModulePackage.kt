package com.reactnativetor

import android.util.Log
import com.facebook.react.bridge.JSIModulePackage
import com.facebook.react.bridge.ReactApplicationContext
import com.facebook.react.bridge.JavaScriptContextHolder
import com.facebook.react.bridge.JSIModuleSpec
import com.reactnativetor.TorModule

class TorModulePackage : JSIModulePackage {
  // init {
    // System.loadLibrary("sifir_android")
  // }
  init {
//    System.loadLibrary("sifir_android")
    // System.loadLibrary("sifir_ios");
    System.loadLibrary("react-native-tor-cpp");
  }

//  private external fun nativeInstall(jsi: Long);

  override fun getJSIModules(
    reactApplicationContext: ReactApplicationContext,
    jsContext: JavaScriptContextHolder
  ): List<JSIModuleSpec<*>> {
//    val jsiPtr = reactApplicationContext.javaScriptContextHolder.get();
//    val tModule = reactApplicationContext.getNativeModule(TorModule::class.java)
    val tModule =  TorModule(reactContext = reactApplicationContext)
    tModule?.installLib(reactContext = jsContext);
//    this.nativeInstall(jsiPtr);
    return emptyList()
  }
}
