/*
   WatchApp: Ripples 3D
   File    : types.h
   Author  : Afonso Santos, Portugal
   Notes   : Dedicated to all the @PebbleDev team and to @KatharineBerry in particular
           : ... for her CloudPebble online dev environment that made this possible.

   Last revision: 17h55 September 22 2016  GMT
*/

#include <pebble.h>


/* -----------   ENUMS   ----------- */

typedef enum { COLORIZATION_UNDEFINED
             , COLORIZATION_MONO
             , COLORIZATION_SIGNAL
             , COLORIZATION_DIST
             , COLORIZATION_SHADOW
             }
Colorization ;


typedef enum { PATTERN_UNDEFINED
             , PATTERN_DOTS
             , PATTERN_LINES
             , PATTERN_STRIPES
             , PATTERN_GRID
             }
Pattern ;


typedef enum { OSCILLATOR_UNDEFINED
             , OSCILLATOR_ANCHORED
             , OSCILLATOR_FLOATING
             , OSCILLATOR_BOUNCING
             }
Oscilator ;


typedef enum { TRANSPARENCY_UNDEFINED
             , TRANSPARENCY_OPAQUE
             , TRANSPARENCY_TRANSLUCENT
             , TRANSPARENCY_XRAY
             }
Transparency ;


/* -----------   STRUCTS   ----------- */

typedef struct
{
  bool fromCam   :1 ;
  bool fromLight1:1 ;
  bool fromLight2:1 ;
  bool fromLight3:1 ;
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


typedef struct
{
  Q3          world ;
  Q           dist2osc ;
  Visibility  visibility ;
  GPoint      screen ;
} Fuxel ;