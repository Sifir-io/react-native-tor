#cmake_minimum_required(VERSION 3.10.2)
#
#set (PACKAGE_NAME "react-native-tor-cpp")
#set (BUILD_DIR ${CMAKE_SOURCE_DIR}/build)
#
#include_directories(
#        "${PACKAGE_NAME}"
#        "${NODE_MODULES_DIR}/react-native/React"
#        "${NODE_MODULES_DIR}/react-native/React/Base"
#        "${NODE_MODULES_DIR}/react-native/ReactCommon/jsi"
#        "${NODE_MODULES_DIR}/react-native/ReactCommon/jsi/jsi"
#        ".src/main/cpp"
#)
#
#file (GLOB LIBRN_DIR "${BUILD_DIR}/react-native-0*/jni/${ANDROID_ABI}")
#
#if (JSI_LIB)
#  # React Native 0.66.x and above
#  add_library(
#          ${PACKAGE_NAME}
#          SHARED
#          ".src/main/cpp/cpp-adapter.cpp"
#  )
#
#  find_library(
#          JSI_LIB
#          jsi
#          PATHS ${LIBRN_DIR}
#          NO_CMAKE_FIND_ROOT_PATH
#  )
#  target_link_libraries(
#          ${PACKAGE_NAME}
#          ${LOG_LIB}
##          ${REACT_NATIVE_JNI_LIB}
#          ${JSI_LIB}
#          android
#  )
#else()
#  # React Native 0.65.x and below
#  add_library(
#          ${PACKAGE_NAME}
#          SHARED
#          ${NODE_MODULES_DIR}/react-native/ReactCommon/jsi/jsi
#          ${NODE_MODULES_DIR}/react-native/ReactCommon/jsi/jsi/jsi.cpp
#          ${NODE_MODULES_DIR}/react-native/ReactCommon/jsi/jsi/jsilib.h
#          ./src/main/cpp/cpp-adapter.cpp
#          ./src/main/cpp
#  )
#  target_link_libraries(
#          ${PACKAGE_NAME}
#          ${LOG_LIB}
##          ${REACT_NATIVE_JNI_LIB}
##          ${JSI_LIB}
#          android
#  )
#endif()
#
#
#find_library(
#        LOG_LIB
#        log
#)
#
##find_library(
##        REACT_NATIVE_JNI_LIB
##        reactnativejni
##        PATHS ${LIBRN_DIR}
##        NO_CMAKE_FIND_ROOT_PATH
##)
#
#set_target_properties(
#        ${PACKAGE_NAME} PROPERTIES
#        CXX_STANDARD 17
#        CXX_EXTENSIONS OFF
#        POSITION_INDEPENDENT_CODE ON
#)
#
##target_link_libraries(
##        ${PACKAGE_NAME}
##        ${LOG_LIB}
##        ${REACT_NATIVE_JNI_LIB}
##        ${JSI_LIB}
##        android
##)
##
#----

cmake_minimum_required(VERSION 3.4.1)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-DFOLLY_NO_CONFIG=1 -DFOLLY_HAVE_CLOCK_GETTIME=1 -DFOLLY_HAVE_MEMRCHR=1 -DFOLLY_USE_LIBCPP=1 -DFOLLY_MOBILE=1")

set(PACKAGE_NAME react-native-tor-cpp)
set(BUILD_DIR ${CMAKE_SOURCE_DIR}/build)
set(RN_SO_DIR "../node_modules/react-native/ReactAndroid/src/main/jni/first-party/react/jni")
set(LIBRN_DIR ${RN_SO_DIR}/${ANDROID_ABI})

if(${REACT_NATIVE_VERSION} VERSION_LESS 0.64.0)
  # for React Native < 0.64.0
  set(TURBOMODULE_DIR ${NODE_MODULES_DIR}/react-native/ReactCommon/turbomodule/core/)
  add_library(turbomodulejsijni SHARED ../cpp/empty.cpp)
else()
  # for React Native >= 0.64.0
  set(TURBOMODULE_DIR ${NODE_MODULES_DIR}/react-native/ReactCommon/react/nativemodule/core/ReactCommon)
endif()

add_library(${PACKAGE_NAME}
        SHARED
        ${NODE_MODULES_DIR}/react-native/ReactCommon/jsi/jsi/jsi.cpp
        ${NODE_MODULES_DIR}/react-native/ReactCommon/jsi/jsi/JSIDynamic.cpp
        ${NODE_MODULES_DIR}/react-native/ReactAndroid/src/main/java/com/facebook/react/turbomodule/core/jni/ReactCommon/CallInvokerHolder.cpp
        ${TURBOMODULE_DIR}/TurboModule.cpp
        ../cpp/c_DataObserver.h
        ../cpp/c_HiddenServiceHandler.h
        ../cpp/c_Logger.h
        ../cpp/c_OwnedTorService.h
        ../cpp/cpp-adapter.cpp
        ../cpp/c_TcpSocksStream.h
        ../cpp/c_TorHiddenService.h
        ../cpp/c_TorServiceParam.h
        ../cpp/DataObserver.hpp
        ../cpp/empty.cpp
        ../cpp/HiddenServiceHandler_fwd.hpp
        ../cpp/HiddenServiceHandler.hpp
        ../cpp/Logger_fwd.hpp
        ../cpp/Logger.hpp
        ../cpp/OwnedTorService_fwd.hpp
        ../cpp/OwnedTorService.hpp
        ../cpp/rust_foreign_slice_impl.hpp
        ../cpp/rust_foreign_slice_iter.hpp
        ../cpp/rust_foreign_vec_impl.hpp
        ../cpp/rust_result4232mut3232c_voidCRustString.h
        ../cpp/rust_slice.h
        ../cpp/rust_slice_mut.h
        ../cpp/rust_slice_tmpl.hpp
        ../cpp/rust_str.h
        ../cpp/rust_vec.h
        ../cpp/rust_vec_impl.hpp
        ../cpp/rust_void_ok_resultCRustString.h
        ../cpp/TcpSocksStream_fwd.hpp
        ../cpp/TcpSocksStream.hpp
        ../cpp/TorHiddenService_fwd.hpp
        ../cpp/TorHiddenService.hpp
        ../cpp/TorServiceParam_fwd.hpp
        ../cpp/TorServiceParam.hpp
        ../cpp/cpp-adapter.cpp
        )

find_package(fbjni REQUIRED CONFIG)

target_include_directories(
        ${PACKAGE_NAME}
        PRIVATE
        ${NODE_MODULES_DIR}/react-native/React
        ${NODE_MODULES_DIR}/react-native/React/Base
        ${NODE_MODULES_DIR}/react-native/ReactCommon
        ${NODE_MODULES_DIR}/react-native/ReactCommon/jsi
        ${NODE_MODULES_DIR}/react-native/ReactCommon/callinvoker
        ${NODE_MODULES_DIR}/react-native/ReactAndroid/src/main/java/com/facebook/react/turbomodule/core/jni/ReactCommon
        ${TURBOMODULE_DIR}
        ${BUILD_DIR}/third-party-ndk/boost/boost_${BOOST_VERSION}
        ${BUILD_DIR}/third-party-ndk/double-conversion
        ${BUILD_DIR}/third-party-ndk/folly
        ${BUILD_DIR}/third-party-ndk/glog/exported
        ../cpp
)

find_library(
        FOLLY_JSON_LIB
        folly_json
        PATHS ${LIBRN_DIR}
        NO_CMAKE_FIND_ROOT_PATH
)

find_library(
        GLOG_LIB
        glog
        PATHS ${LIBRN_DIR}
        NO_CMAKE_FIND_ROOT_PATH
)

target_link_libraries(${PACKAGE_NAME}
        fbjni::fbjni
        log
        ${FOLLY_JSON_LIB}
        ${GLOG_LIB}
        ${PROJECT_SOURCE_DIR}/../android/libs/bin/${CMAKE_ANDROID_ARCH_ABI}/libsifir_android.so
        )
# ----

#cmake_minimum_required(VERSION 3.4.1)
#
#set(CMAKE_VERBOSE_MAKEFILE ON)
#set(CMAKE_CXX_STANDARD 17)
#set(CMAKE_CXX_FLAGS "-DFOLLY_NO_CONFIG=1 -DFOLLY_HAVE_CLOCK_GETTIME=1 -DFOLLY_HAVE_MEMRCHR=1 -DFOLLY_USE_LIBCPP=1 -DFOLLY_MOBILE=1")
#
#set(PACKAGE_NAME react-native-tor-cpp)
#set(BUILD_DIR ${CMAKE_SOURCE_DIR}/build)
#set(RN_SO_DIR "../node_modules/react-native/ReactAndroid/src/main/jni/first-party/react/jni")
#set(LIBRN_DIR ${RN_SO_DIR}/${ANDROID_ABI})
#
#if(${REACT_NATIVE_VERSION} VERSION_LESS 0.64.0)
#  # for React Native < 0.64.0
#  set(TURBOMODULE_DIR ${NODE_MODULES_DIR}/react-native/ReactCommon/turbomodule/core/)
#  add_library(turbomodulejsijni SHARED ../cpp/empty.cpp)
#else()
#  # for React Native >= 0.64.0
#  set(TURBOMODULE_DIR ${NODE_MODULES_DIR}/react-native/ReactCommon/react/nativemodule/core/ReactCommon)
#endif()
#
#include_directories(
#        "${PACKAGE_NAME}"
#        "${NODE_MODULES_DIR}/react-native/React"
#        "${NODE_MODULES_DIR}/react-native/React/Base"
#        "${NODE_MODULES_DIR}/react-native/ReactCommon/jsi"
#        "${NODE_MODULES_DIR}/react-native/ReactCommon/jsi/jsi"
#        ${NODE_MODULES_DIR}/react-native/ReactAndroid/src/main/java/com/facebook/react/turbomodule/core/jni/ReactCommon
#        ${NODE_MODULES_DIR}/react-native/ReactAndroid/src/main/jni/third-party
#        ".src/main/cpp"
#)
#
#add_library(${PACKAGE_NAME}
#        SHARED
#        ${NODE_MODULES_DIR}/react-native/ReactCommon/jsi/jsi
#        ${NODE_MODULES_DIR}/react-native/ReactCommon/jsi/jsi/jsilib.h
#        ${NODE_MODULES_DIR}/react-native/ReactCommon/jsi/jsi/jsilib.h
#        # --
#        ${NODE_MODULES_DIR}/react-native/ReactCommon/jsi/jsi/jsi.cpp
##        ${NODE_MODULES_DIR}/react-native/ReactCommon/jsi/jsi/JSIDynamic.cpp
##        ${NODE_MODULES_DIR}/react-native/ReactAndroid/src/main/java/com/facebook/react/turbomodule/core/jni/ReactCommon/CallInvokerHolder.cpp
##        ${NODE_MODULES_DIR}/react-native/ReactAndroid/src/main/java/com/facebook/react/turbomodule/core/jni/ReactCommon/CallInvokerHolder.h
##        ${NODE_MODULES_DIR}/react-native/ReactAndroid/src/main/jni/third-party/folly/dynamic.h
#        ${TURBOMODULE_DIR}/TurboModule.cpp
##        ../cpp/Blob.cpp
##        ../cpp/Blob.h
##        ../cpp/BlobManager.cpp
##        ../cpp/BlobManager.h
##        ../cpp/base64.h
##        ../cpp/NativeModules.cpp
##        ../cpp/NativeModules.h
##        ../cpp/TonClientJsiModule.h
##        ../cpp/request_data_t.h
##        ../cpp/tonclient.h
##        ../cpp/TonClientJsiModule.cpp
#        ../cpp/cpp-adapter.cpp
#        )
#
