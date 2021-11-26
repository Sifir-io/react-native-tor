#cmake_minimum_required(VERSION 3.10.2)
#
#set (CMAKE_VERBOSE_MAKEFILE ON)
#set (CMAKE_CXX_STANDARD 17)
#set (CMAKE_CXX_FLAGS "-DFOLLY_NO_CONFIG=1 -DFOLLY_HAVE_CLOCK_GETTIME=1 -DFOLLY_HAVE_MEMRCHR=1 -DFOLLY_USE_LIBCPP=1 -DFOLLY_MOBILE=1 -DON_ANDROID")
#
#set(PACKAGE_NAME "react-native-tor")
#set(build_DIR ${CMAKE_SOURCE_DIR}/build)
#
#add_library(
#        ${PACKAGE_NAME}
#        SHARED
##        ${NODE_MODULES_DIR}/react-native/ReactCommon/jsi/jsi/jsi.cpp
#        ./cpp-adapter.cpp
#)
#
#include_directories(
#        ${PACKAGE_NAME}
#        PRIVATE
#        "${NODE_MODULES_DIR}/react-native/React"
#        "${NODE_MODULES_DIR}/react-native/React/Base"
#        "${NODE_MODULES_DIR}/react-native/ReactAndroid/src/main/jni"
#        "${NODE_MODULES_DIR}/react-native/ReactAndroid/src/main/java/com/facebook/react/turbomodule/core/jni"
#        "${NODE_MODULES_DIR}/react-native/ReactCommon"
#        "${NODE_MODULES_DIR}/react-native/ReactCommon/callinvoker"
#        "${NODE_MODULES_DIR}/react-native/ReactCommon/jsi"
#)
#
#file(GLOB LIBRN_DIR "${BUILD_DIR}/react-native-0*/jni/${ANDROID_ABI}")
##find_library(
##        log-lib
##        log
##)
#
#find_library(
#        REACT_NATIVE_JNI_LIB
#        reactnativejni
#        PATHS ${LIBRN_DIR}
#        NO_CMAKE_FIND_ROOT_PATH
#)
#
#
## find_package(fbjni REQUIRED CONFIG)
#
#set_target_properties(
#        react-native-tor PROPERTIES
#        CXX_STANDARD 17
#        CXX_EXTENSIONS OFF
#        POSITION_INDEPENDENT_CODE ON
#)
#
#file(GLOB libfbjni_link_DIRS "${build_DIR}/fbjni*.aar/jni/${ANDROID_ABI}")
#file(GLOB libfbjni_include_DIRS "${build_DIR}/fbjni-*-headers.jar/")
#
#
#find_library(FBJNI_LIBRARY fbjni PATHS ${libfbjni_link_DIRS} NO_CMAKE_FIND_ROOT_PATH)
#target_include_directories(${PACKAGE_NAME} PRIVATE ${libfbjni_include_DIRS})
#
#
##target_link_libraries(${PACKAGE_NAME} core ${log-lib} ${REACT_NATVE_JNI_LIB} android)
#target_link_libraries(${PACKAGE_NAME} PRIVATE ${REACT_NATVE_JNI_LIB} android)
cmake_minimum_required(VERSION 3.9.0)

set (PACKAGE_NAME "react-native-tor")
set (BUILD_DIR ${CMAKE_SOURCE_DIR}/build)

include_directories(
        "${PACKAGE_NAME}"
        "${NODE_MODULES_DIR}/react-native/React"
        "${NODE_MODULES_DIR}/react-native/React/Base"
        "${NODE_MODULES_DIR}/react-native/ReactCommon/jsi"
        ".src/main/cpp/cpp-adapter.cpp"
)

file (GLOB LIBRN_DIR "${BUILD_DIR}/react-native-0*/jni/${ANDROID_ABI}")

if (JSI_LIB)
  # React Native 0.66.x and above
  add_library(
          ${PACKAGE_NAME}
          SHARED
          ".src/main/cpp/cpp-adapter.cpp"
  )

  find_library(
          JSI_LIB
          jsi
          PATHS ${LIBRN_DIR}
          NO_CMAKE_FIND_ROOT_PATH
  )
  target_link_libraries(
          ${PACKAGE_NAME}
          ${LOG_LIB}
#          ${REACT_NATIVE_JNI_LIB}
          ${JSI_LIB}
          android
  )
else()
  # React Native 0.65.x and below
  add_library(
          ${PACKAGE_NAME}
          SHARED
          ../node_modules/react-native/ReactCommon/jsi/jsi
          ../node_modules/react-native/ReactCommon/jsi/jsi/jsi.cpp
#          ../../react-native/ReactCommon/jsi/jsi/jsi.cpp
          ./src/main/cpp/cpp-adapter.cpp
  )
  target_link_libraries(
          ${PACKAGE_NAME}
          ${LOG_LIB}
#          ${REACT_NATIVE_JNI_LIB}
#          ${JSI_LIB}
          android
  )
endif()


find_library(
        LOG_LIB
        log
)

#find_library(
#        REACT_NATIVE_JNI_LIB
#        reactnativejni
#        PATHS ${LIBRN_DIR}
#        NO_CMAKE_FIND_ROOT_PATH
#)

set_target_properties(
        ${PACKAGE_NAME} PROPERTIES
        CXX_STANDARD 17
        CXX_EXTENSIONS OFF
        POSITION_INDEPENDENT_CODE ON
)

#target_link_libraries(
#        ${PACKAGE_NAME}
#        ${LOG_LIB}
#        ${REACT_NATIVE_JNI_LIB}
#        ${JSI_LIB}
#        android
#)
