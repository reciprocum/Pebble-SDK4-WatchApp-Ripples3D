/*
   WatchApp: Ripples 3D
   File    : Config.h
   Author  : Afonso Santos, Portugal
   Notes   : Dedicated to all the @PebbleDev team and to @KatharineBerry in particular
           : ... for her CloudPebble online dev environment that made this possible.

   Last revision: 13h45 September 27 2016  GMT
*/

#pragma once

// Uncommenting the next line will enable all LOG* calls.
//#define LOG

// Uncommenting the next line will enable GIF mode.
//#define  GIF
//#define  GIF_STOP_COUNT     2720

// Commenting the next line will enable fast distro settings
//#define EMU

// Uncoment next line to use BASALT to "fake" running on APLITE/DIORITE B&W platforms with antialising on ;-)
//#undef PBL_COLOR

#ifdef LOG
  #define LOGD(fmt, ...) APP_LOG(APP_LOG_LEVEL_DEBUG, fmt, ##__VA_ARGS__)
  #define LOGI(fmt, ...) APP_LOG(APP_LOG_LEVEL_INFO, fmt, ##__VA_ARGS__)
  #define LOGW(fmt, ...) APP_LOG(APP_LOG_LEVEL_WARNING, fmt, ##__VA_ARGS__)
  #define LOGE(fmt, ...) APP_LOG(APP_LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__)
#else
  #define LOGD(fmt, ...)
  #define LOGI(fmt, ...)
  #define LOGW(fmt, ...)
  #define LOGE(fmt, ...)
#endif