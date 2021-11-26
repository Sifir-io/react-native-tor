package com.reactnativetor

import com.facebook.react.bridge.JSIModulePackage
import com.facebook.react.bridge.ReactApplicationContext
import com.facebook.react.bridge.JavaScriptContextHolder
import com.facebook.react.bridge.JSIModuleSpec
import com.reactnativetor.TorModule

class TorModulePackage : JSIModulePackage {
    override fun getJSIModules(
        reactApplicationContext: ReactApplicationContext,
        jsContext: JavaScriptContextHolder
    ): List<JSIModuleSpec<*>> {
        reactApplicationContext.getNativeModule(TorModule::class.java)!!.installLib(jsContext)
        return emptyList()
    }
}
