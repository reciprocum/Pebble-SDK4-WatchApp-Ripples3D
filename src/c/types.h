/*
   WatchApp: Ripples 3D
   File    : types.h
   Author  : Afonso Santos, Portugal
   Notes   : Dedicated to all the @PebbleDev team and to @KatharineBerry in particular
           : ... for her CloudPebble online dev environment that made this possible.

   Last revision: 21h45 September 28 2016  GMT
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
             , COLORIZATION_MONOCHROMATIC
             , COLORIZATION_DISTANCE
             }
Colorization ;


typedef enum { ILLUMINATION_UNDEFINED
             , ILLUMINATION_DIFUSE
             , ILLUMINATION_SPOTLIGHT
             }
Illumination ;


typedef enum { DETAIL_UNDEFINED
             , DETAIL_COARSE
             , DETAIL_FINE
             }
Detail ;


/* -----------   STRUCTS   ----------- */

typedef struct
{
  bool cam      :1 ;
  bool spotlight:1 ;
} Visibility ;


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
  Q3          world ;
  Q           dist2osc ;
  Visibility  visibility ;
  GPoint      screen ;
  union Pen   pen ;
} Fuxel ;