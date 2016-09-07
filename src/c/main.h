/*
   WatchApp: Ripples 3D
   File    : main.h
   Author  : Afonso Santos, Portugal
   Notes   : Dedicated to all the @PebbleDev team and to @KatharineBerry in particular
           : ... for her CloudPebble online dev environment that made this possible.

   Last revision: 13h45 September 07 2016  GMT
*/

#include "Config.h"

// World related

#ifdef PBL_PLATFORM_APLITE
  #define GRID_LINES        27
#else
  #define GRID_LINES        31
#endif

#define  GRID_SCALE                 6.28f
#define  CAM3D_DISTANCEFROMORIGIN   8.75f

#define  PLOTTER_MODE_DEFAULT       PLOTTER_MODE_LINES

#ifdef PBL_COLOR
  #define  COLOR_MODE_DEFAULT       COLOR_MODE_DIST
#else
  #define  COLOR_MODE_DEFAULT       COLOR_MODE_MONO
#endif

#ifdef GIF
  #define  ANTIALIASING_DEFAULT     true
  #define  ANIMATION_INTERVAL_MS    0
#else
  #define  ANTIALIASING_DEFAULT     false

  #ifdef EMU
    #define  ANIMATION_INTERVAL_MS    150
  #else
    #define  ANIMATION_INTERVAL_MS    35
  #endif
#endif

// Animation related: adds wrist movement reaction inertia to dampen accelerometer jerkiness.
#define ACCEL_SAMPLER_CAPACITY    8
