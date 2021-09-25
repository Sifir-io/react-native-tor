cmake_minimum_required(VERSION 3.10.2)

set (CMAKE_VERBOSE_MAKEFILE ON)
set (CMAKE_CXX_STANDARD 14)
set (CMAKE_CXX_FLAGS "-DFOLLY_NO_CONFIG=1 -DFOLLY_HAVE_CLOCK_GETTIME=1 -DFOLLY_HAVE_MEMRCHR=1 -DFOLLY_USE_LIBCPP=1 -DFOLLY_MOBILE=1 -DON_ANDROID")

set(build_DIR ${CMAKE_SOURCE_DIR}/build)
set(PACKAGE_NAME "react-native-tor")

find_package(fbjni REQUIRED CONFIG)

include_directories(
        ${PACKAGE_NAME}
        PRIVATE
        "${NODE_MODULES_DIR}/react-native/React"
        "${NODE_MODULES_DIR}/react-native/React/Base"
#        "${NODE_MODULES_DIR}/react-native/ReactAndroid/src/main/jni"
#        "${NODE_MODULES_DIR}/react-native/ReactAndroid/src/main/java/com/facebook/react/turbomodule/core/jni"
#        "${NODE_MODULES_DIR}/react-native/ReactCommon"
#        "${NODE_MODULES_DIR}/react-native/ReactCommon/callinvoker"
        "${NODE_MODULES_DIR}/react-native/ReactCommon/jsi"
)

add_library(
        ${PACKAGE_NAME}
        SHARED
        ./cpp-adapter.cpp
)

set_target_properties(
        react-native-tor PROPERTIES
        CXX_STANDARD 17
        CXX_EXTENSIONS OFF
        POSITION_INDEPENDENT_CODE ON
)

file (GLOB LIBRN_DIR "${BUILD_DIR}/react-native-0*/jni/${ANDROID_ABI}")

find_library(
        log-lib
        log
)

find_library(
        REACT_NATIVE_JNI_LIB
        reactnativejni
        PATHS ${LIBRN_DIR}
        NO_CMAKE_FIND_ROOT_PATH
)


target_link_libraries(${PACKAGE_NAME} core ${log-lib} ${REACT_NATVE_JNI_LIB})
