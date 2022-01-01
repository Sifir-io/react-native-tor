#include <jni.h>
#include <jsi/jsi.h>
#include <sys/types.h>
#include "pthread.h"
#include<unordered_map>
#include <android/log.h>
#include <sifir-tor.h>
#include <sifir-typedef.h>

using namespace facebook;
using namespace std;
using namespace libsifir;

JavaVM *java_vm;
jclass java_class;
jobject java_object;
unordered_map<long, shared_ptr<jsi::Function>> cb_map;
OwnedTorService *service;

/**
 * A simple callback function that allows us to detach current JNI Environment
 * when the thread
 * See https://stackoverflow.com/a/30026231 for detailed explanation
 */

void DeferThreadDetach(JNIEnv *env) {
    static pthread_key_t thread_key;

    // Set up a Thread Specific Data key, and a callback that
    // will be executed when a thread is destroyed.
    // This is only done once, across all threads, and the value
    // associated with the key for any given thread will initially
    // be NULL.
    static auto run_once = [] {
        const auto err = pthread_key_create(&thread_key, [](void *ts_env) {
            if (ts_env) {
                java_vm->DetachCurrentThread();
            }
        });
        if (err) {
            // Failed to create TSD key. Throw an exception if you want to.
        }
        return 0;
    }();

    // For the callback to actually be executed when a thread exits
    // we need to associate a non-NULL value with the key on that thread.
    // We can use the JNIEnv* as that value.
    const auto ts_env = pthread_getspecific(thread_key);
    if (!ts_env) {
        if (pthread_setspecific(thread_key, env)) {
            // Failed to set thread-specific value for key. Throw an exception if you want to.
        }
    }
}

/**
 * Get a JNIEnv* valid for this thread, regardless of whether
 * we're on a native thread or a Java thread.
 * If the calling thread is not currently attached to the JVM
 * it will be attached, and then automatically detached when the
 * thread is destroyed.
 *
 * See https://stackoverflow.com/a/30026231 for detailed explanation
 */
JNIEnv *GetJniEnv() {
    JNIEnv *env = nullptr;
    // We still call GetEnv first to detect if the thread already
    // is attached. This is done to avoid setting up a DetachCurrentThread
    // call on a Java thread.

    // g_vm is a global.
    auto get_env_result = java_vm->GetEnv((void **) &env, JNI_VERSION_1_6);
    if (get_env_result == JNI_EDETACHED) {
        if (java_vm->AttachCurrentThread(&env, NULL) == JNI_OK) {
            DeferThreadDetach(env);
        } else {
            // Failed to attach thread. Throw an exception if you want to.
        }
    } else if (get_env_result == JNI_EVERSION) {
        // Unsupported JNI version. Throw an exception if you want to.
    }
    return env;
}


static jstring string2jstring(JNIEnv *env, const string &str) {
    return (*env).NewStringUTF(str.c_str());
}

static string jstring2string(JNIEnv *env, jstring jStr) {
    if (!jStr)
        return "";

    const jclass stringClass = env->GetObjectClass(jStr);
    const jmethodID getBytes = env->GetMethodID(stringClass, "getBytes", "(Ljava/lang/String;)[B");
    const jbyteArray stringJbytes = (jbyteArray) env->CallObjectMethod(jStr, getBytes,
                                                                       env->NewStringUTF("UTF-8"));

    size_t length = (size_t) env->GetArrayLength(stringJbytes);
    jbyte *pBytes = env->GetByteArrayElements(stringJbytes, NULL);

    std::string ret = std::string((char *) pBytes, length);
    env->ReleaseByteArrayElements(stringJbytes, pBytes, JNI_ABORT);

    env->DeleteLocalRef(stringJbytes);
    env->DeleteLocalRef(stringClass);
    return ret;
}


void install(facebook::jsi::Runtime &jsiRuntime) {
    auto request = jsi::Function::createFromHostFunction(jsiRuntime,
                                                         jsi::PropNameID::forAscii(jsiRuntime,
                                                                                   "request"),
                                                         6, [](jsi::Runtime &runtime,
                                                               const jsi::Value &thisValue,
                                                               const jsi::Value *arguments,
                                                               size_t count) -> jsi::Value {

                JNIEnv *jniEnv = GetJniEnv();
                java_class = jniEnv->GetObjectClass(java_object);
                jmethodID get = jniEnv->GetMethodID(java_class, "request",
                                                    "(DLjava/lang/String;JJ)I");

                string url = arguments[0].getString(runtime).utf8(runtime);
                string method = arguments[1].getString(runtime).utf8(runtime);
                string jsonBody = arguments[2].getString(runtime).utf8(runtime);
//                auto requestHeaders = arguments[3].getObject(runtime).asHostObject(runtime);
                bool trustAllSSl = arguments[4].getBool();
                // cb_map["bar"] = move(arguments[5].getObject(runtime).getHostObject(runtime));
                int result = jniEnv->CallIntMethod(java_object, get, string2jstring(jniEnv, url),
                                                   string2jstring(jniEnv, method),
                                                   string2jstring(jniEnv, jsonBody),
                                                   true,
//                                                   requestHeaders,
                                                   string2jstring(jniEnv, "bar"));
                return jsi::Value();

            });
    //auto startDaemon = jsi::Function::createFromHostFunction(jsiRuntime,
    //                                                         jsi::PropNameID::forAscii(jsiRuntime,
    //                                                                                   "startDaemon"),
    //                                                         2, [](jsi::Runtime &runtime,
    //                                                               const jsi::Value &thisValue,
    //                                                               const jsi::Value *arguments,
    //                                                               size_t count) -> jsi::Value {

    //            __android_log_write(android_LogPriority::ANDROID_LOG_DEBUG, "react_tor_cpp",
    //                                "->startDaemon");
    //            JNIEnv *jniEnv = GetJniEnv();
    //            java_class = jniEnv->GetObjectClass(java_object);
    //            jmethodID get = jniEnv->GetMethodID(java_class, "startDaemon",
    //                                                "(DJ)I");

    //            double timeout = arguments[0].getNumber();
    //            shared_ptr<jsi::Function> cb_ptr = std::make_shared<jsi::Function>(
    //                    move(arguments[1].getObject(runtime).asFunction(runtime)));
    //            // increase ref count and store in map
    //            auto *ptr = cb_ptr.get();
    //            cb_map[reinterpret_cast<long>(ptr)] = cb_ptr;
    //            int result = jniEnv->CallIntMethod(java_object, get, timeout, ptr);
    //            __android_log_write(android_LogPriority::ANDROID_LOG_DEBUG, "react_tor_cpp",
    //                                "<-startDaemon");
    //            return jsi::Value(result);
    //        });
    auto startDaemon = jsi::Function::createFromHostFunction(jsiRuntime,
                                                             jsi::PropNameID::forAscii(jsiRuntime,
                                                                                       "startDaemon"),
                                                             2, [](jsi::Runtime &runtime,
                                                                   const jsi::Value &thisValue,
                                                                   const jsi::Value *arguments,
                                                                   size_t count) -> jsi::Value {

                __android_log_write(android_LogPriority::ANDROID_LOG_DEBUG, "react_tor_cpp",
                                    "->startDaemon");
                // FIXME add methods to get app path and get port


                if (service) {
                    __android_log_write(android_LogPriority::ANDROID_LOG_ERROR, "react_tor_cpp",
                                        "Daemon already running! stopDaemon first");
                    return jsi::Value(-1);
                }
                double timeout = arguments[0].getNumber();
                shared_ptr<jsi::Function> cb_ptr = std::make_shared<jsi::Function>(
                        move(arguments[1].getObject(runtime).asFunction(runtime)));
                // increase ref count and store in map
                auto *ptr = cb_ptr.get();
                cb_map[reinterpret_cast<long>(ptr)] = cb_ptr;
                auto result = get_owned_TorService(
                        "//data/user/0/com.example.reactnativetor/cache/sifir_sdk/tor/", 41944,
                        timeout);

                if (result->message.tag == ResultMessage_Tag::Success) {

                    service = result->result;
                    __android_log_write(android_LogPriority::ANDROID_LOG_DEBUG, "react_tor_cpp",
                                        "<-startDaemon");
                    if (ptr->isFunction(runtime)) {
                        __android_log_write(android_LogPriority::ANDROID_LOG_DEBUG, "react_tor_cpp",
                                            "-- calling cb");
                        ptr->call(runtime, 41943);
                    }

                    return jsi::Value(0);
                } else {
                    // TODO return error message
                    return jsi::Value(-1);
                }
            }
    );


    auto stopDaemon = jsi::Function::createFromHostFunction(jsiRuntime,
                                                            jsi::PropNameID::forAscii(jsiRuntime,
                                                                                      "stopDaemon"),
                                                            1, [](jsi::Runtime &runtime,
                                                                  const jsi::Value &thisValue,
                                                                  const jsi::Value *arguments,
                                                                  size_t count) -> jsi::Value {

                __android_log_write(android_LogPriority::ANDROID_LOG_DEBUG, "react_tor_cpp",
                                    "->stopDaemon");

                if (!service) {
                    __android_log_write(android_LogPriority::ANDROID_LOG_DEBUG, "react_tor_cpp",
                                        "Service not init! Start deamon first");
                    return jsi::Value(-1);
                }
                shared_ptr<jsi::Function> cb_ptr = std::make_shared<jsi::Function>(
                        move(arguments[0].getObject(runtime).asFunction(runtime)));
                // increase ref count and store in map
                libsifir::shutdown_owned_TorService(service);
                service = nullptr;
                __android_log_write(android_LogPriority::ANDROID_LOG_DEBUG, "react_tor_cpp",
                                    "<-stopDaemon");
                return jsi::Value(0);
            });
    auto getDaemonStatus = jsi::Function::createFromHostFunction(jsiRuntime,
                                                                 jsi::PropNameID::forAscii(
                                                                         jsiRuntime,
                                                                         "getDaemonStatus"),
                                                                 1, [](jsi::Runtime &runtime,
                                                                       const jsi::Value &thisValue,
                                                                       const jsi::Value *arguments,
                                                                       size_t count) -> jsi::Value {

                __android_log_write(android_LogPriority::ANDROID_LOG_DEBUG, "react_tor_cpp",
                                    "->getDaemonStatus");
                if (!service) {
                    __android_log_write(android_LogPriority::ANDROID_LOG_DEBUG, "react_tor_cpp",
                                        "Service not init! Start deamon first");
                    return jsi::Value(jsi::String::createFromAscii(runtime, "NOTINIT"));
                }
                auto status = libsifir::get_status_of_owned_TorService(service);
                __android_log_write(android_LogPriority::ANDROID_LOG_DEBUG, "react_tor_cpp",
                                    "<-getDaemonStatus");
                return jsi::Value(jsi::String::createFromAscii(runtime, status));
            });
    jsi::Object module = jsi::Object(jsiRuntime);

    module.setProperty(jsiRuntime, "startDaemon", move(startDaemon));
    module.setProperty(jsiRuntime, "stopDaemon", move(stopDaemon));
    module.setProperty(jsiRuntime, "getDaemonStatus", move(getDaemonStatus));
    module.setProperty(jsiRuntime, "request", move(request));
    jsiRuntime.global().setProperty(jsiRuntime, "TorBridge", move(module));

}

extern "C"
JNIEXPORT void JNICALL
Java_com_reactnativetor_TorModule_nativeInstall(JNIEnv *env, jobject thiz, jlong jsi) {
    auto runtime = reinterpret_cast<facebook::jsi::Runtime *>(jsi);
    if (runtime) {
        install(*runtime);
    }
    env->GetJavaVM(&java_vm);
    java_object = env->NewGlobalRef(thiz);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_reactnativetor_TorModule_torCbInt(JNIEnv *env, jobject thiz, jlong jsi, jlong ptr,
                                           jint payload) {
    __android_log_write(android_LogPriority::ANDROID_LOG_DEBUG, "react_tor_cpp", "->torCb");
    auto cb = cb_map[ptr];
    auto runtime = reinterpret_cast<jsi::Runtime *>(jsi);
    if (cb->isFunction(*runtime)) {
        __android_log_write(android_LogPriority::ANDROID_LOG_DEBUG, "react_tor_cpp",
                            "-- calling cb");
        cb->call(*runtime, jsi::Value(*runtime, payload));
    } else {
        __android_log_write(android_LogPriority::ANDROID_LOG_ERROR, "react_tor_cpp", "not cb!");
    }
    cb_map.erase(ptr);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_reactnativetor_TorModule_torCbString(JNIEnv *env, jobject thiz, jlong jsi, jlong ptr,
                                              jstring payload) {
    __android_log_write(android_LogPriority::ANDROID_LOG_DEBUG, "react_tor_cpp", "->torCbString");
    auto cb = cb_map[ptr];
    auto runtime = reinterpret_cast<jsi::Runtime *>(jsi);
    if (cb->isFunction(*runtime)) {
        __android_log_write(android_LogPriority::ANDROID_LOG_DEBUG, "react_tor_cpp",
                            "-- calling cb");
        auto msg = jstring2string(env, payload).c_str();
        cb->call(*runtime, jsi::String::createFromUtf8(*runtime, msg));
    } else {
        __android_log_write(android_LogPriority::ANDROID_LOG_ERROR, "react_tor_cpp", "not cb!");
    }
    cb_map.erase(ptr);
}
extern "C"
JNIEXPORT void JNICALL
Java_com_reactnativetor_TorModule_torCbMap(JNIEnv *env, jobject thiz, jlong jsi, jlong ptr,
                                           jobject payload) {
    __android_log_write(android_LogPriority::ANDROID_LOG_DEBUG, "react_tor_cpp", "->torCbMap");
    auto cb = cb_map[ptr];
    auto runtime = reinterpret_cast<jsi::Runtime *>(jsi);
    if (cb->isFunction(*runtime)) {
        __android_log_write(android_LogPriority::ANDROID_LOG_DEBUG, "react_tor_cpp",
                            "-- calling cb");
//        cb->call(*runtime, jsi::Object::(*runtime, payload));
    } else {
        __android_log_write(android_LogPriority::ANDROID_LOG_ERROR, "react_tor_cpp", "not cb!");
    }
    cb_map.erase(ptr);
}
extern "C"
JNIEXPORT void JNICALL
Java_com_reactnativetor_TorModule_torErrCb(JNIEnv *env, jobject thiz, jlong jsi, jlong ptr,
                                           jstring message) {
    __android_log_write(android_LogPriority::ANDROID_LOG_DEBUG, "react_tor_cpp", "->torErrCb");
    auto runtime = reinterpret_cast<jsi::Runtime *>(jsi);
    auto msg = jstring2string(env, message);
    __android_log_write(android_LogPriority::ANDROID_LOG_DEBUG, "react_tor_cpp", "--throwing err");
    cb_map.erase(ptr);
    jsi::detail::throwJSError(*runtime, msg.c_str());
}
