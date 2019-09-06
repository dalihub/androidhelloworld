# androidhelloworld
Android native activty with DALi hello world example

Project structure:
app
 |_src
 |  |_main
 |     |_assets (All hello world specific assets, images, models - none so far)
 |     |_cpp (Contains hello-world-example.cpp, copy of dali/hello-world/app/src/main/cpp/hello-world-example.cpp)
 |     |_res
 |
 |_dali (DALi folder, containes toolkit assets, fonts, libdali.so, headers)
    |_include (public DAli includes, android_native_dali_app_glue.h - Android DALi glue for native activity)
    |_lib (libdali.so, release/debug)
    |_assets (DALi toolkit images, styles, fonts)


The hello-world-example.cpp is modified to include android_native_dali_app_glue.h, everything else is the same.
