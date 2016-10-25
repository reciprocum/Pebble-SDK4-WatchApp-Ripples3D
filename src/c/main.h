/*
   WatchApp: Ripples 3D
   File    : main.h
   Author  : Afonso Santos, Portugal
   Notes   : Dedicated to all the @PebbleDev team and to @KatharineBerry in particular
           : ... for her CloudPebble online dev environment that made this possible.

   Last revision: 25 October 2016
*/

#include "Config.h"


/* -----------   User   ----------- */

#define USER_SECONDSINACTIVE_MAX       120


/* -----------   Default modes   ----------- */

#if defined(GIF)
  #define  OSCILLATOR_DEFAULT      OSCILLATOR_BOUNCING
  #define  PATTERN_DEFAULT         PATTERN_STRIPES
  #define  TRANSPARENCY_DEFAULT    TRANSPARENCY_OPAQUE
  #define  COLORIZATION_DEFAULT    COLORIZATION_DISTANCE
  #define  ILLUMINATION_DEFAULT    ILLUMINATION_SPOTLIGHT
  #define  DETAIL_DEFAULT          DETAIL_FINE
  #define  ANIMATION_INTERVAL_MS   100

  #define VISIBILITY_MAX_ITERATIONS   5
  #define LINE_PRECISION_PXL          2
#else
  #define  OSCILLATOR_DEFAULT      OSCILLATOR_ANCHORED
  #define  PATTERN_DEFAULT         PATTERN_LINES
  #define  TRANSPARENCY_DEFAULT    TRANSPARENCY_TRANSLUCENT

  #if defined(PBL_COLOR)
    #define  COLORIZATION_DEFAULT     COLORIZATION_DISTANCE
  #else
    #define  COLORIZATION_DEFAULT     COLORIZATION_MONOCHROMATIC
  #endif

  #define  ILLUMINATION_DEFAULT     ILLUMINATION_DIFUSE
  #define  DETAIL_DEFAULT           DETAIL_COARSE

  #if defined(EMU)
    #define  ANIMATION_INTERVAL_MS    80
  #else
    #define  ANIMATION_INTERVAL_MS    50
  #endif

  #define VISIBILITY_MAX_ITERATIONS   4
  #define LINE_PRECISION_PXL          3
#endif


// Animation related: adds wrist movement reaction inertia to dampen accelerometer jerkiness.
#define ACCEL_SAMPLER_CAPACITY    8


/* -----------   GRID/CAMERA PARAMETERS   ----------- */

#if defined(PBL_PLATFORM_APLITE)
  #define  GRID_LINES     23
#else
  #define  GRID_LINES     27
#endif

// The GRID_SCALE value bellow has been precison engineered as to saturate x,y grid coord tables in signed Q3.12 format (int16_t),
// make 100% SURE you do the proper (required) adjustments if you ever change this value.
#define  GRID_SCALE                 7.9999f

#define  CAM3D_DISTANCEFROMORIGIN   9.75f
#define  LIGHT_DISTANCEFROMORIGIN   5.25f


/* -----------   PHYSICS PARAMETERS   ----------- */

//  With a lubrication value of 6 drag is 1/2^6 (1/64 ~1.5%) of speed. Should kill all speed in about 100 frames (~4s)
//  Increase this value for the speed to last longer
//  Decrease this value for the speed to dissipate faster
#define OSCILLATOR_LUBRICATION_LEVEL    6

//  Controls how fast a wrist tilt will influence the oscilator horizontal moving speed.
//  Increase this value for a "heavier" feeling
//  Decrease this value for a "lighter" feeling
#define OSCILLATOR_INERTIA_LEVEL        1

//  Controls how fast the oscillator cycles.
//  Increase this value for a "faster" wave
//  Decrease this value for a "slower" wave
#define OSCILLATOR_PHASE_SPEED        10


