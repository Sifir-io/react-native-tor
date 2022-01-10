#include <jni.h>
#include <jsi/jsi.h>
#include <sys/types.h>
#include "pthread.h"
#include<unordered_map>
#include <android/log.h>
#include <sifir-tor.h>
#include <sifir-typedef.h>
#include <thread>
#include <future>

using namespace facebook;
using namespace std;
using namespace libsifir;

JavaVM *java_vm;
jclass java_class;
jobject java_object;
unordered_map<long, shared_ptr<jsi::Function>> cb_map;
unordered_map<long, shared_ptr<std::function<void(char *payload)>>> fn_wrapper;
OwnedTorService *service;
int socks_port = 41945;

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

static jobject jsObjectToJStringHashMap(JNIEnv *env, jsi::Runtime &rt, const jsi::Object &obj) {
    auto fields = obj.getPropertyNames(rt);
    jsize map_len = fields.size(rt);

    auto classMap = env->FindClass("java/util/HashMap");
    // java_class = env->GetObjectClass(classMap);
    jmethodID init = env->GetMethodID(classMap, "<init>", "(I)V");
    jobject hashMap = env->NewObject(classMap, init, map_len);
    jmethodID put = env->GetMethodID(classMap, "put",
                                     "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");
    for (int i = 0; i < fields.size(rt); i++) {
        auto field_name = fields.getValueAtIndex(rt, i).asString(rt);
        auto value = obj.getProperty(rt, field_name).asString(rt);
        __android_log_write(android_LogPriority::ANDROID_LOG_DEBUG, "react_tor_cpp",
                            "->JNI");
        env->CallObjectMethod(hashMap, put, string2jstring(env, field_name.utf8(rt)),
                              string2jstring(env, value.utf8(rt)));
    }
    return hashMap;
}

static void torCbString(long cb_index, const void *jsi, char *payload) {
    __android_log_write(android_LogPriority::ANDROID_LOG_DEBUG, "react_tor_cpp", "->torCbString");

    auto cb = cb_map[cb_index];

    void *q = const_cast<void *>(jsi);
    auto runtime = reinterpret_cast<jsi::Runtime *>(q);

    if (cb->isFunction(*runtime)) {
        __android_log_write(android_LogPriority::ANDROID_LOG_DEBUG, "react_tor_cpp",
                            "-- calling cb");
        cb->call(*runtime, jsi::String::createFromUtf8(*runtime, payload));
    } else {
        __android_log_write(android_LogPriority::ANDROID_LOG_ERROR, "react_tor_cpp", "not cb!");
    }
    cb_map.erase(cb_index);
}

void install(facebook::jsi::Runtime &jsiRuntime) {
    auto request = jsi::Function::createFromHostFunction(jsiRuntime,
                                                         jsi::PropNameID::forAscii(jsiRuntime,
                                                                                   "request"),
                                                         6, [](jsi::Runtime &runtime,
                                                               const jsi::Value &thisValue,
                                                               const jsi::Value *arguments,
                                                               size_t count) -> jsi::Value {

                __android_log_write(android_LogPriority::ANDROID_LOG_DEBUG, "react_tor_cpp",
                                    "->request");
                if (!service) {
                    __android_log_write(android_LogPriority::ANDROID_LOG_ERROR, "react_tor_cpp",
                                        "Daemon not running! startDaemon first");
                    return jsi::Value(-1);
                }
                string url = arguments[0].getString(runtime).utf8(runtime);

                shared_ptr<jsi::Function> cb_ptr = std::make_shared<jsi::Function>(
                        arguments[5].getObject(runtime).asFunction(runtime));
                auto *ptr = cb_ptr.get();
                long cb_index = reinterpret_cast<long>(ptr);
                cb_map[cb_index] = cb_ptr;

                __android_log_write(android_LogPriority::ANDROID_LOG_DEBUG, "react_tor_cpp",
                                    "-request:callInMethod");
                // HERE -- START OF OBSERVER
                Observer observer = Observer();
                // context capturing wrapper
                shared_ptr<std::function<void(
                        char *payload)>> success_ptr = make_shared<std::function<void(
                        char *payload)>>([&runtime, cb_index](char *payload) {
                    __android_log_write(android_LogPriority::ANDROID_LOG_DEBUG, "react_tor_cpp",
                                        "->observer::wrapper");
                    torCbString(cb_index, &runtime, payload);
                    __android_log_write(android_LogPriority::ANDROID_LOG_DEBUG, "react_tor_cpp",
                                        "<-observer::wrapper");
                });
                auto *wrapper_ptr = success_ptr.get();
                long wrapper_index = reinterpret_cast<long>(wrapper_ptr);
                fn_wrapper[wrapper_index] = success_ptr;

                observer.context = wrapper_ptr;

                auto on_err = [](char *payload, const void *stuff) {
                    __android_log_write(android_LogPriority::ANDROID_LOG_DEBUG, "react_tor_cpp",
                                        "-request:lamda got err paylaod");
                };
                auto on_success = [](char *payload, const void *stuff) {
                    __android_log_write(android_LogPriority::ANDROID_LOG_DEBUG, "react_tor_cpp",
                                        "->on_success");
                    __android_log_write(android_LogPriority::ANDROID_LOG_DEBUG, "react_tor_cpp",
                                        payload);
                    void *q = const_cast<void *>(stuff);
                    long index = reinterpret_cast<long>(q);
                    auto wrapper = fn_wrapper[index].get();
                    (*wrapper)(payload);
                    __android_log_write(android_LogPriority::ANDROID_LOG_DEBUG, "react_tor_cpp",
                                        "<-on_success");
                };
                observer.on_err = on_err;
                observer.on_success = on_success;
                // EOF OBSERVER
                libsifir::get(socks_port, url.c_str(), observer);
                __android_log_write(android_LogPriority::ANDROID_LOG_DEBUG, "react_tor_cpp",
                                    "<-request");
                return jsi::Value(0);

            });
    //auto request = jsi::Function::createFromHostFunction(jsiRuntime,
    //                                                     jsi::PropNameID::forAscii(jsiRuntime,
    //                                                                               "request"),
    //                                                     6, [](jsi::Runtime &runtime,
    //                                                           const jsi::Value &thisValue,
    //                                                           const jsi::Value *arguments,
    //                                                           size_t count) -> jsi::Value {

    //            // FIXME here, maybe this is the one function we can reuse the local impl ?
    //            // rather than having to package and rework all this into the rust..?
    //            __android_log_write(android_LogPriority::ANDROID_LOG_DEBUG, "react_tor_cpp",
    //                                "->request");
    //            if (!service) {
    //                __android_log_write(android_LogPriority::ANDROID_LOG_ERROR, "react_tor_cpp",
    //                                    "Daemon not running! startDaemon first");
    //                return jsi::Value(-1);
    //            }
    //            JNIEnv *jniEnv = GetJniEnv();
    //            java_class = jniEnv->GetObjectClass(java_object);
    //            jmethodID get = jniEnv->GetMethodID(java_class, "request",
    //                                                "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/util/HashMap;ZIJ)I");

    //            jstring url = string2jstring(jniEnv, arguments[0].getString(runtime).utf8(runtime));
    //            jstring method = string2jstring(jniEnv,
    //                                            arguments[1].getString(runtime).utf8(runtime));
    //            jstring jsonBody = string2jstring(jniEnv,
    //                                              arguments[2].getString(runtime).utf8(runtime));
    //            __android_log_write(android_LogPriority::ANDROID_LOG_DEBUG, "react_tor_cpp",
    //                                "-request:prepareHeaders");
    //            auto requestHeaders = jsObjectToJStringHashMap(jniEnv, runtime,
    //                                                           arguments[3].getObject(runtime));
    //            bool trustAllSSl = arguments[4].getBool();
    //            shared_ptr<jsi::Function> cb_ptr = std::make_shared<jsi::Function>(
    //                    arguments[5].getObject(runtime).asFunction(runtime));
    //            auto *ptr = cb_ptr.get();
    //            cb_map[reinterpret_cast<long>(ptr)] = cb_ptr;
    //            __android_log_write(android_LogPriority::ANDROID_LOG_DEBUG, "react_tor_cpp",
    //                                "-request:callInMethod");


    //            int result = jniEnv->CallIntMethod(java_object, get,
    //                                               url,
    //                                               method,
    //                                               jsonBody,
    //                                               requestHeaders,
    //                                               trustAllSSl,
    //                                               socks_port,
    //                                               ptr);
    //            __android_log_write(android_LogPriority::ANDROID_LOG_DEBUG, "react_tor_cpp",
    //                                "<-request");
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
                // get free port
                JNIEnv *jniEnv = GetJniEnv();
                java_class = jniEnv->GetObjectClass(java_object);
                jmethodID get = jniEnv->GetMethodID(java_class, "findFreePort", "()I");

                // set global here
                socks_port = jniEnv->CallIntMethod(java_object, get);
                __android_log_write(android_LogPriority::ANDROID_LOG_DEBUG, "react_tor_cpp",
                                    "startDaemon::got socks port");

                int timeout = arguments[0].getNumber();
                shared_ptr <jsi::Function> cb_ptr = std::make_shared<jsi::Function>(
                        move(arguments[1].getObject(runtime).asFunction(runtime)));
                // increase ref count and store in map
                auto *ptr = cb_ptr.get();
                auto cb_index = reinterpret_cast<long>(ptr);
                cb_map[cb_index] = cb_ptr;

                std::promise<BoxedResult_OwnedTorService *> service_promise;
                std::future<BoxedResult_OwnedTorService *> tor_future = service_promise.get_future();
                auto handler = std::thread([](int sp, int to,
                                              std::promise<BoxedResult_OwnedTorService *> &service_promise) {
                    __android_log_write(android_LogPriority::ANDROID_LOG_DEBUG, "react_tor_cpp",
                                        "- startDaemon::thread");
                    auto r = get_owned_TorService("//data/user/0/com.example.reactnativetor", sp,
                                                  to);
                    __android_log_write(android_LogPriority::ANDROID_LOG_DEBUG, "react_tor_cpp",
                                        "- startDaemon::thread::got service");
                    service_promise.set_value(r);
                    __android_log_write(android_LogPriority::ANDROID_LOG_DEBUG, "react_tor_cpp",
                                        "<- startDaemon::thread");
                }, socks_port, timeout, std::ref(service_promise));

                // await result
                auto result = tor_future.get();
                if (result->message.tag == ResultMessage_Tag::Success) {
                    service = result->result;
                    __android_log_write(android_LogPriority::ANDROID_LOG_DEBUG, "react_tor_cpp",
                                        "<-startDaemon");
                    if (ptr->isFunction(runtime)) {
                        __android_log_write(android_LogPriority::ANDROID_LOG_DEBUG, "react_tor_cpp",
                                            "-- calling cb");
                        ptr->call(runtime, jsi::Value(socks_port));
                    }
                } else {
                    // TODO return error message throw
                    ptr->call(runtime, "balls");
                }
                handler.join();
                return jsi::Value(-1);
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
    libsifir::init_logger();
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
