/*
   WatchApp: Ripples 3D
   File    : Config.h
   Author  : Afonso Santos, Portugal

   Last revision: 20h34 September 03 2016  GMT
*/

#pragma once

// Uncommenting the next line will enable all LOG* calls.
//#define LOG

// Uncommenting the next line will enable GIF mode.
//#define GIF

// Commenting the next line will enable fast distro settings
//#define EMU

// Coment to enable antialiasing
//#define RAW

// Uncoment next line to "fake" running on APLITE/DIORITE B&W platforms.
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