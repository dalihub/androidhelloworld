cmake_minimum_required(VERSION 3.4.1)

set(DALI_ANDROID "${CMAKE_CURRENT_SOURCE_DIR}/../../../../dali")

# build native_app_glue as a static lib
set(${CMAKE_C_FLAGS}, "${CMAKE_C_FLAGS}")
add_library(native_app_glue STATIC
    ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)

add_definitions(-DAPPLICATION_RESOURCE_PATH=\"assets/resources/\")

# now build app's shared lib
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions -frtti -w -Wall -std=gnu++11")

# Export ANativeActivity_onCreate(),
# Refer to: https://github.com/android-ndk/ndk/issues/381.
set(CMAKE_SHARED_LINKER_FLAGS
    "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")

add_library(native-activity SHARED
        hello-world-example.cpp)

target_include_directories(native-activity PRIVATE
    ${ANDROID_NDK}/sources/android/native_app_glue)

# DALi
include_directories(${DALI_ANDROID}/include)
include_directories(${DALI_ANDROID}/include/dali)

add_library(dali
        SHARED
        IMPORTED )

set_target_properties(dali
        PROPERTIES IMPORTED_LOCATION
        ${DALI_ANDROID}/lib/${ANDROID_ABI}/libdali.so )

# add lib dependencies
target_link_libraries(native-activity
        dali
        log
        android
        native_app_glue
        EGL
        GLESv3)
