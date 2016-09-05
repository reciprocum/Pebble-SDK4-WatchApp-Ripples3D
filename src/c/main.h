/*
   WatchApp: Ripples 3D
   File    : main.h
   Author  : Afonso Santos, Portugal

   Last revision: 22h04 September 04 2016  GMT
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

#define  RENDER_MODE_DEFAULT        RENDER_MODE_LINES
#define  COLOR_MODE_DEFAULT         COLOR_MODE_DIST

// Animation related: determines movement inertia.
#define ACCEL_SAMPLER_CAPACITY    8

#ifdef EMU
  #define  ANIMATION_INTERVAL_MS    100
#else
  #define  ANIMATION_INTERVAL_MS    40
#endif


//void  click_config_provider( void *context ) ;
//void  world_stop( ) ;
//void  world_finalize( ) ;