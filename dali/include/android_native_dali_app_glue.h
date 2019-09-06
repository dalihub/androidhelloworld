/*
 * Copyright (c) 2019 Samsung Electronics Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

// EXTERNAL INCLUDES
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <android/log.h>
#include <android_native_app_glue.h>
#include <dali/devel-api/adaptor-framework/application-devel.h>
#include <dali/integration-api/debug.h>

// from android_native_app_glue.c
#ifndef NDEBUG
#  define LOGV(...)  ((void)__android_log_print(ANDROID_LOG_VERBOSE, "dali", __VA_ARGS__))
#else
#  define LOGV(...)  ((void)0)
#endif

namespace
{
    void free_saved_state(struct android_app *android_app) {
        pthread_mutex_lock(&android_app->mutex);
        if (android_app->savedState != NULL) {
            free(android_app->savedState);
            android_app->savedState = NULL;
            android_app->savedStateSize = 0;
        }
        pthread_mutex_unlock(&android_app->mutex);
    }

    void android_app_destroy(struct android_app *android_app) {
        LOGV("android_app_destroy!");
        free_saved_state(android_app);
        pthread_mutex_lock(&android_app->mutex);
        if (android_app->inputQueue != NULL) {
            AInputQueue_detachLooper(android_app->inputQueue);
        }
        AConfiguration_delete(android_app->config);
        android_app->destroyed = 1;
        pthread_cond_broadcast(&android_app->cond);
        pthread_mutex_unlock(&android_app->mutex);
        // Can't touch android_app object after this.
    }
}

void AutoHideNavBar(struct android_app* state)
{
    JNIEnv* env;
    state->activity->vm->AttachCurrentThread(&env, NULL);

    jclass activityClass = env->FindClass("android/app/NativeActivity");
    jmethodID getWindow = env->GetMethodID(activityClass, "getWindow", "()Landroid/view/Window;");

    jclass windowClass = env->FindClass("android/view/Window");
    jmethodID getDecorView = env->GetMethodID(windowClass, "getDecorView", "()Landroid/view/View;");

    jclass viewClass = env->FindClass("android/view/View");
    jmethodID setSystemUiVisibility = env->GetMethodID(viewClass, "setSystemUiVisibility", "(I)V");

    jobject window = env->CallObjectMethod(state->activity->clazz, getWindow);

    jobject decorView = env->CallObjectMethod(window, getDecorView);

    jfieldID flagFullscreenID = env->GetStaticFieldID(viewClass, "SYSTEM_UI_FLAG_FULLSCREEN", "I");
    jfieldID flagHideNavigationID = env->GetStaticFieldID(viewClass, "SYSTEM_UI_FLAG_HIDE_NAVIGATION", "I");
    jfieldID flagImmersiveStickyID = env->GetStaticFieldID(viewClass, "SYSTEM_UI_FLAG_IMMERSIVE_STICKY", "I");

    const int flagFullscreen = env->GetStaticIntField(viewClass, flagFullscreenID);
    const int flagHideNavigation = env->GetStaticIntField(viewClass, flagHideNavigationID);
    const int flagImmersiveSticky = env->GetStaticIntField(viewClass, flagImmersiveStickyID);
    const int flag = flagFullscreen | flagHideNavigation | flagImmersiveSticky;

    env->CallVoidMethod(decorView, setSystemUiVisibility, flag);

    state->activity->vm->DetachCurrentThread();
}

void ExtractAsset( struct android_app* state, std::string assetPath, std::string filePath )
{
    AAsset* asset = AAssetManager_open( state->activity->assetManager, assetPath.c_str(), AASSET_MODE_BUFFER );
    if( asset )
    {
        size_t length = AAsset_getLength( asset ) + 1;

        char* buffer = new char[ length ];
        length = AAsset_read( asset, buffer, length );

        FILE* file = fopen( filePath.c_str(), "wb" );
        if( file )
        {
          fwrite( buffer, 1, length, file );
          fclose( file );
        }

        delete[] buffer;
        AAsset_close( asset );
    }
}

void ExtractAssets( struct android_app* state, std::string assetDirPath, std::string filesDirPath )
{
  AAssetDir* assetDir = AAssetManager_openDir( state->activity->assetManager, assetDirPath.c_str() );
  if( assetDir )
  {
    if( mkdir( filesDirPath.c_str(), S_IRWXU ) != -1 )
    {
      const char *filename = NULL;
      std::string assetPath = assetDirPath + "/";
      while ( ( filename = AAssetDir_getNextFileName( assetDir ) ) != NULL )
      {
        ExtractAsset( state, assetPath + filename, filesDirPath + "/" + filename );
      }
    }

    AAssetDir_close( assetDir );
  }
}

void ExtractFontConfig( struct android_app* state, std::string assetFontConfig, std::string fontsPath )
{
  AAsset* asset = AAssetManager_open( state->activity->assetManager, assetFontConfig.c_str(), AASSET_MODE_BUFFER );
  if( asset )
  {
    size_t length = AAsset_getLength( asset ) + 1;

    char* buffer = new char[ length ];
    length = AAsset_read( asset, buffer, length );

    std::string fontConfig = std::string( buffer, length );
    int i = fontConfig.find("<dir></dir>");
    if( i != std::string::npos )
      fontConfig.replace( i + strlen("<dir>"), 0, fontsPath );

    i = fontConfig.find("<cachedir></cachedir>");
    if( i != std::string::npos )
      fontConfig.replace( i + strlen("<cachedir>"), 0, fontsPath );

    std::string fontsFontConfig = fontsPath + "/fonts.conf";
    FILE* file = fopen( fontsFontConfig.c_str(), "wb" );
    if( file )
    {
      fwrite( fontConfig.c_str(), 1, fontConfig.size(), file );
      fclose( file );
    }

    delete[] buffer;
    AAsset_close( asset );
  }
}

extern "C" void FcConfigPathInit(const char* path, const char* file);

int main( int argc, char **argv );

void android_main( struct android_app* state )
{
    std::string filesDir = state->activity->internalDataPath;

    struct stat st = { 0 };
    std::string fontconfigPath = filesDir + "/fonts";
    setenv( "FONTCONFIG_PATH", fontconfigPath.c_str(), 1 );
    std::string fontconfigFile = fontconfigPath + "/fonts.conf";
    setenv( "FONTCONFIG_FILE", fontconfigFile.c_str(), 1 );
    FcConfigPathInit( fontconfigPath.c_str(), fontconfigFile.c_str() );

    if( stat( fontconfigPath.c_str(), &st ) == -1 )
    {
       mkdir( fontconfigPath.c_str(), S_IRWXU );
       ExtractFontConfig( state, "fonts/fonts.conf", fontconfigPath );
       ExtractAssets( state, "fonts/dejavu", fontconfigPath + "/dejavu" );
       ExtractAssets( state, "fonts/tizen", fontconfigPath + "/tizen" );
    }

    AutoHideNavBar( state );

    Dali::DevelApplication::SetApplicationContext( state );

    int status = main( 0, nullptr );

    android_app_destroy( state );

    // TODO: We need to kill the application process manually, DALi cannot restart in the same process due to memory leaks
    std::exit( status );
}

//END_INCLUDE(all)
