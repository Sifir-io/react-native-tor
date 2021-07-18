package com.reactnativetor

import java.util.Arrays
import java.util.Collections

import com.facebook.react.ReactPackage
import com.facebook.react.bridge.NativeModule
import com.facebook.react.bridge.ReactApplicationContext
import com.facebook.react.uimanager.ViewManager
import com.facebook.react.bridge.JavaScriptModule
import android.content.pm.PackageManager

class TorPackage : ReactPackage {

  override fun createNativeModules(reactContext: ReactApplicationContext): List<NativeModule> {
    val manager = reactContext.getPackageManager();
    val ai = manager.getApplicationInfo(reactContext.packageName, PackageManager.GET_META_DATA);
    System.loadLibrary("sifir_android")
//    System.load("${ai.nativeLibraryDir}/libsifir_android.so");
    return Arrays.asList<NativeModule>(TorModule(reactContext))
  }

  override fun createViewManagers(reactContext: ReactApplicationContext): List<ViewManager<*, *>> {
    return emptyList<ViewManager<*, *>>()
  }
}
