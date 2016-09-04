/*
   WatchApp: Ripples 3D
   File    : main.h
   Author  : Afonso Santos, Portugal

   Last revision: 23h04 September 03 2016  GMT
*/

#include "Config.h"

// World related

#ifdef PBL_PLATFORM_APLITE
  #define GRID_LINES        27
#else
  #define GRID_LINES        31
#endif

#define GRID_SCALE             6.28f

// Animation related
#define ACCEL_SAMPLER_CAPACITY    8

#ifdef EMU
  #define  ANIMATION_INTERVAL_MS    175
#else
  #define  ANIMATION_INTERVAL_MS    40
#endif

#define  RENDER_MODE_DEFAULT        RENDER_MODE_LINES
#define  CAM3D_DISTANCEFROMORIGIN   (Q_from_float(8.75f))


typedef enum { RENDER_MODE_UNDEFINED
             , RENDER_MODE_DOTS
             , RENDER_MODE_LINES
             }
RenderMode ;


static void  set_render_mode( RenderMode pRenderMode ) ;
void  click_config_provider( void *context ) ;
