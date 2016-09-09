/*
   WatchApp: Ripples 3D
   File    : main.h
   Author  : Afonso Santos, Portugal
   Notes   : Dedicated to all the @PebbleDev team and to @KatharineBerry in particular
           : ... for her CloudPebble online dev environment that made this possible.

   Last revision: 15h35 September 09 2016  GMT
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

#ifdef GIF
  #define  ANTIALIASING_DEFAULT     true
  #define  ANIMATION_INTERVAL_MS    20
#else
  #define  ANTIALIASING_DEFAULT     false

  #ifdef EMU
    #define  ANIMATION_INTERVAL_MS    120
  #else
    #define  ANIMATION_INTERVAL_MS    35
  #endif
#endif

// Animation related: adds wrist movement reaction inertia to dampen accelerometer jerkiness.
#define ACCEL_SAMPLER_CAPACITY    8

#define  PLOTTER_MODE_DEFAULT       PLOTTER_MODE_LINES

#ifdef PBL_COLOR
  #define  COLOR_MODE_DEFAULT       COLOR_MODE_DIST
#else
  #define  COLOR_MODE_DEFAULT       COLOR_MODE_MONO
#endif

#define  OSCILLATOR_MODE_DEFAULT     OSCILLATOR_MODE_ANCHORED
//#define  OSCILLATOR_MODE_DEFAULT     OSCILLATOR_MODE_FLOATING
//#define  OSCILLATOR_MODE_DEFAULT     OSCILLATOR_MODE_BOUNCING
