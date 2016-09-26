/*
   WatchApp: Ripples 3D
   File    : types.h
   Author  : Afonso Santos, Portugal
   Notes   : Dedicated to all the @PebbleDev team and to @KatharineBerry in particular
           : ... for her CloudPebble online dev environment that made this possible.

   Last revision: 12h15 September 26 2016  GMT
*/

#include <pebble.h>
#include <karambola/Draw2D.h>


/* -----------   ENUMS   ----------- */

typedef enum { PATTERN_UNDEFINED
             , PATTERN_DOTS
             , PATTERN_LINES
             , PATTERN_STRIPES
             , PATTERN_GRID
             }
Pattern ;


typedef enum { TRANSPARENCY_UNDEFINED
             , TRANSPARENCY_TRANSLUCENT
             , TRANSPARENCY_XRAY
             , TRANSPARENCY_OPAQUE
             }
Transparency ;


typedef enum { OSCILLATOR_UNDEFINED
             , OSCILLATOR_ANCHORED
             , OSCILLATOR_FLOATING
             , OSCILLATOR_BOUNCING
             }
Oscillator ;


typedef enum { COLORIZATION_UNDEFINED
             , COLORIZATION_MONO
             , COLORIZATION_SIGNAL
             , COLORIZATION_DIST
             }
Colorization ;


typedef enum { ILLUMINATION_UNDEFINED
             , ILLUMINATION_DIFUSE
             , ILLUMINATION_SHADOW
             , ILLUMINATION_SPOTLIGHTS
             }
Illumination ;


/* -----------   STRUCTS   ----------- */

union Visibility
{
  struct
  {
    bool cam   :1 ;
    bool light1:1 ;
    bool light2:1 ;
    bool light3:1 ;
  }         from ;
  uint8_t   value ;
} ;


typedef struct
{
  bool xMajor:1 ;
  bool xMinor:1 ;
  bool yMajor:1 ;
  bool yMinor:1 ;
  bool zMajor:1 ;
  bool zMinor:1 ;
} Boxing ;


union Pen
{
  GColor  color ;
  ink_t   ink ;
} ;


typedef struct
{
  Q3                world ;
  Q                 dist2osc ;
  union Visibility  visibility ;
  GPoint            screen ;
  union Pen         pen ;
} Fuxel ;