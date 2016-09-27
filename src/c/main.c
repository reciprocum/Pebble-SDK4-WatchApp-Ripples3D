/*
   WatchApp: Ripples 3D
   File    : main.c
   Author  : Afonso Santos, Portugal
   Notes   : Dedicated to all the @PebbleDev team and to @KatharineBerry in particular
           : ... for her CloudPebble online dev environment that made this possible.

   Last revision: 13h45 September 27 2016  GMT
*/

#include <pebble.h>
#include <karambola/Q2.h>
#include <karambola/Q3.h>
#include <karambola/CamQ3.h>
#include <karambola/Sampler.h>
#include <karambola/Draw2D.h>

#include "main.h"
#include "Config.h"
#include "types.h"


/* -----------   GLOBAL VARS   ----------- */

// UI related
static Window         *s_window ;
static Layer          *s_window_layer ;
static Layer          *s_world_layer ;
static ActionBarLayer *s_action_bar_layer ;

// World related
const  Q    grid_scale     = Q_from_float(GRID_SCALE) ;
const  Q    grid_halfScale = Q_from_float(GRID_SCALE) >> 1 ;   //  scale / 2

static Pattern       s_pattern       = PATTERN_UNDEFINED ;
static Transparency  s_transparency  = TRANSPARENCY_UNDEFINED ;
static Oscillator    s_oscillator    = OSCILLATOR_UNDEFINED ;
static Colorization  s_colorization  = COLORIZATION_UNDEFINED ;
static Illumination  s_illumination  = ILLUMINATION_UNDEFINED ;

static Q    world_xMin, world_xMax, world_yMin, world_yMax, world_zMin, world_zMax ;


Boxing
world_boxing( Q3 p )
{
  Boxing boxing ;

  boxing.xMajor = (p.x > world_xMax) ;
  boxing.xMinor = (p.x < world_xMin) ;
  boxing.yMajor = (p.y > world_yMax) ;
  boxing.yMinor = (p.y < world_yMin) ;
  boxing.zMajor = (p.z > world_zMax) ;
  boxing.zMinor = (p.z < world_zMin) ;
  
  return boxing ;
}


#define COORD_SHIFT  4
#define Z_SHIFT      9
#define DIST_SHIFT   4

static int16_t     grid_major_x         [GRID_LINES] ;                   // S3.12  Coords [-7.999,+7.999]
static int16_t     grid_major_y         [GRID_LINES] ;                   // S3.12  Coords [-7.999,+7.999]
static int8_t      grid_major_z         [GRID_LINES][GRID_LINES] ;       // S0.7   f(x,y) [-0.99, +0.99]
static uint16_t    grid_major_dist2osc  [GRID_LINES][GRID_LINES] ;       // U4.12  Need integer part up to 11.3137 because of max diagonal distance for bouncing oscillator.
static Visibility  grid_major_visibility[GRID_LINES][GRID_LINES] ;
static GPoint      grid_major_screen    [GRID_LINES][GRID_LINES] ;

static int16_t     grid_minor_x         [GRID_LINES-1] ;                 // S3.12  Coords [-7.999,+7.999]
static int16_t     grid_minor_y         [GRID_LINES-1] ;                 // S3.12  Coords [-7.999,+7.999]
static int8_t      grid_minor_z         [GRID_LINES-1][GRID_LINES-1] ;   // S0.7   f(x,y) [-0.99, +0.99]
static uint16_t    grid_minor_dist2osc  [GRID_LINES-1][GRID_LINES-1] ;   // U4.12  Need integer part up to sqrt(2) * GRID_SCALE because of max diagonal distance for bouncing oscillator.
static Visibility  grid_minor_visibility[GRID_LINES-1][GRID_LINES-1] ;
static GPoint      grid_minor_screen    [GRID_LINES-1][GRID_LINES-1] ;

static int32_t oscillator_anglePhase ;
static Q2      oscillator_position ;
static Q2      oscillator_speed ;          // For OSCILLATOR_BOUNCING
static Q2      oscillator_acceleration ;   // For OSCILLATOR_BOUNCING

static int        s_world_updateCount       = 0 ;
static AppTimer  *s_world_updateTimer_ptr   = NULL ;


/***  ---------------  Prototypes  ---------------  ***/

void grid_dist2osc_update( ) ;
void position_setFromSensors( Q2 *positionPtr ) ;
void acceleration_setFromSensors( Q2 *accelerationPtr ) ;

void
cam_config
( const Q3       viewPoint
, const int32_t  rotZangle
, const int32_t  rotXangle
) ;

void grid_minor_dist2osc_update( ) ;
void grid_minor_z_update( ) ;
void grid_minor_visibility_update( ) ;


/***  ---------------  PATTERN  ---------------  ***/

void
pattern_set
( Pattern pattern )
{
  if (s_pattern == pattern)
    return ;

  switch (s_pattern = pattern)
  {
    case PATTERN_DOTS:
    case PATTERN_STRIPES:
      grid_minor_dist2osc_update( ) ;
      grid_minor_z_update( ) ;
      grid_minor_visibility_update( ) ;
    break ;

    case PATTERN_LINES:
    case PATTERN_GRID:
    case PATTERN_UNDEFINED:
    break ;
  }
}


void
pattern_change
( )
{
  switch (s_pattern)
  {
      case PATTERN_DOTS:
        pattern_set( PATTERN_LINES ) ;
      break ;

      case PATTERN_LINES:
        pattern_set( PATTERN_STRIPES ) ;
      break ;

      case PATTERN_STRIPES:
        pattern_set( PATTERN_GRID ) ;
      break ;

      case PATTERN_GRID:
        pattern_set( PATTERN_DOTS ) ;
      break ;

      case PATTERN_UNDEFINED:
        pattern_set( PATTERN_DEFAULT ) ;
      break ;
  }

  layer_mark_dirty( s_world_layer ) ;
}


void
pattern_change_click_handler
( ClickRecognizerRef recognizer
, void              *context
)
{ pattern_change( ) ; }


/***  ---------------  TRANSPARENCY  ---------------  ***/

void
transparency_set
( Transparency pTransparency )
{
  if (s_transparency == pTransparency)
    return ;

  switch (s_transparency = pTransparency)
  {
    case TRANSPARENCY_TRANSLUCENT:
      // Set all major to true.
      for (int i = 0  ;  i < GRID_LINES  ;  ++i)
        for (int j = 0  ;  j < GRID_LINES  ;  ++j)
          grid_major_visibility[i][j].cam = true ;

      // Set all minor to true.
      for (int i = 0  ;  i < GRID_LINES-1  ;  ++i)
        for (int j = 0  ;  j < GRID_LINES-1  ;  ++j)
          grid_minor_visibility[i][j].cam = true ;
    break ;

    case TRANSPARENCY_XRAY:
    case TRANSPARENCY_OPAQUE:
    // To be updated elsewere.
    break ;

    case TRANSPARENCY_UNDEFINED:
    break ;
  } ;
}


void
transparency_change
( )
{
  // Cycle trough the transparency modes.
  switch (s_transparency)
  {
    case TRANSPARENCY_TRANSLUCENT:
      transparency_set( TRANSPARENCY_XRAY ) ;
    break ;

    case TRANSPARENCY_XRAY:
      transparency_set( TRANSPARENCY_OPAQUE ) ;
    break ;

    case TRANSPARENCY_OPAQUE:
      transparency_set( TRANSPARENCY_TRANSLUCENT ) ;
    break ;

    case TRANSPARENCY_UNDEFINED:
      transparency_set( TRANSPARENCY_DEFAULT ) ;
    break ;
  } ;

  layer_mark_dirty( s_world_layer ) ;
}


void
transparency_change_click_handler
( ClickRecognizerRef recognizer
, void              *context
)
{ transparency_change( ) ; }


/***  ---------------  COLORIZATION  ---------------  ***/

void
colorization_set
( Colorization colorization )
{
  if (s_colorization == colorization)
    return ;

  s_colorization = colorization ;
}


void
colorization_change
( )
{
  // Cycle trough the color modes.
  switch (s_colorization)
  {
    case COLORIZATION_MONOCHROMATIC:
      colorization_set( COLORIZATION_DISTANCE ) ;
    break ;

    case COLORIZATION_DISTANCE:
      colorization_set( COLORIZATION_MONOCHROMATIC ) ;
    break ;

    case COLORIZATION_UNDEFINED:
      colorization_set( COLORIZATION_DEFAULT ) ;
    break ;
  } ;

  layer_mark_dirty( s_world_layer ) ;
}


void
colorization_change_click_handler
( ClickRecognizerRef recognizer
, void              *context
)
{ colorization_change( ) ; }


/***  ---------------  ILUMINATION  ---------------  ***/

void
illumination_set
( Illumination ilumination )
{
  if (s_illumination == ilumination)
    return ;

  s_illumination = ilumination ;
}


void
illumination_change
( )
{
  // Cycle trough the ilumination modes.
  switch (s_illumination)
  {
    case ILLUMINATION_UNDEFINED:
      illumination_set( ILLUMINATION_DEFAULT ) ;
    break ;

    case ILLUMINATION_DIFUSE:
      illumination_set( ILLUMINATION_SPOTLIGHT ) ;
    break ;

    case ILLUMINATION_SPOTLIGHT:
      illumination_set( ILLUMINATION_DIFUSE ) ;
    break ;
  }

  layer_mark_dirty( s_world_layer ) ;
}


void
illumination_change_click_handler
( ClickRecognizerRef recognizer
, void              *context
)
{ illumination_change( ) ; }


/***  ---------------  Camera related  ---------------  ***/

static CamQ3   s_cam ;
static Q3      s_cam_viewPoint ;
static Boxing  s_cam_viewPoint_boxing ;
static Q       s_cam_zoom           = PBL_IF_RECT_ELSE(Q_from_float(+1.25f), Q_from_float(+1.05f)) ;
static int32_t s_cam_rotZangle      = 0
             , s_cam_rotXangle      = 0
             , s_cam_rotZangleSpeed = 0
             , s_cam_rotXangleSpeed = 0
             ;


void
cam_initialize
( )
{
  // Initialize cam rotation vars.
  s_cam_rotZangle = s_cam_rotXangle = 0 ;

  switch (s_oscillator)
  {
    case OSCILLATOR_ANCHORED:
      s_cam_rotZangleSpeed = TRIG_MAX_ANGLE >> 9 ;   //  2 * PI / 512
      s_cam_rotXangleSpeed = 0 ;

/*
      #ifdef GIF
        s_cam_rotXangleSpeed = TRIG_MAX_ANGLE >> 11 ;
      #else
        s_cam_rotXangleSpeed = 0 ;
      #endif
*/
    break ;

    case OSCILLATOR_FLOATING:
    case OSCILLATOR_BOUNCING:
    case OSCILLATOR_UNDEFINED:
      s_cam_rotZangleSpeed = s_cam_rotXangleSpeed = 0 ;
    break ;
  }
}


void
cam_config
( const Q3       viewPoint
, const int32_t  rotZangle
, const int32_t  rotXangle
)
{
  Q3 transformedVP ;

  Q3_scaTo( &transformedVP, Q_from_float(CAM3D_DISTANCEFROMORIGIN), &viewPoint ) ;

  if (rotZangle != 0)
    Q3_rotZ( &transformedVP, &transformedVP, rotZangle ) ;

  if (rotXangle != 0)
    Q3_rotX( &transformedVP, &transformedVP, rotXangle ) ;

  // setup 3D camera
  CamQ3_lookAtOriginUpwards( &s_cam
                           , (transformedVP.x != Q_0  ||  transformedVP.y != Q_0)        // Viewpoint on Z axis ?
                             ? &transformedVP                                            // Not on Z axis: use it.
                             : &(Q3){ .x = Q_1>>4, .y = Q_1>>4, .z = transformedVP.z }   // On Z axis: use epsilon close alternative instead.
                           , s_cam_zoom
                           , CAM_PROJECTION_PERSPECTIVE
                           ) ;

  s_cam_viewPoint_boxing = world_boxing( s_cam.viewPoint ) ;
}


/***  ---------------  Illumination related  ---------------  ***/

static Q3      s_spotlight        ;
static Boxing  s_spotlight_boxing ;


void
light_initialize
( )
{
  Q3_set( &s_spotlight, Q_from_float( +0.5f ), Q_from_float( -0.25f ), Q_from_float( +0.375f ) ) ;
  Q3_scaTo( &s_spotlight, Q_from_float(LIGHT_DISTANCEFROMORIGIN), &s_spotlight ) ;
  s_spotlight_boxing = world_boxing( s_spotlight ) ;
}


/***  ---------------  Screen related  ---------------  ***/

static Q   screen_project_scale ;
static Q2  screen_project_translate ;


void
screen_project
( GPoint *screen, const Q3 world )
{
  // Calculate (c)amera film plane 2D coordinates of 3D (w)orld points.
  Q2 camera ;  CamQ3_view( &camera, &s_cam, &world ) ;

  // Convert camera coordinates to screen/device coordinates.
  const Q screenX = Q_mul( screen_project_scale, camera.x ) + screen_project_translate.x ;  screen->x = Q_to_int(screenX) ;
  const Q screenY = Q_mul( screen_project_scale, camera.y ) + screen_project_translate.y ;  screen->y = Q_to_int(screenY) ;
}


/***  ---------------  OSCILLATOR  ---------------  ***/

void
oscillator_set
( Oscillator oscilator )
{
  if (s_oscillator == oscilator)
    return ;

  switch (s_oscillator = oscilator)
  {
    case OSCILLATOR_ANCHORED:
    case OSCILLATOR_UNDEFINED:
      s_cam_rotZangle = s_cam_rotXangle = 0 ;
      oscillator_position = Q2_origin ;   //  oscillator_position := Q2_origin
    break ;

    case OSCILLATOR_FLOATING:
      Q3_set( &s_cam_viewPoint, Q_from_float( +0.1f ), Q_from_float( -1.0f ), Q_from_float( +0.7f ) ) ;
      cam_config( s_cam_viewPoint, s_cam_rotZangle = 0, s_cam_rotXangle = 0 ) ;
      position_setFromSensors( &oscillator_position ) ;
    break ;

    case OSCILLATOR_BOUNCING:
      Q3_set( &s_cam_viewPoint, Q_from_float( +0.1f ), Q_from_float( -1.0f ), Q_from_float( +0.7f ) ) ;
      cam_config( s_cam_viewPoint, s_cam_rotZangle = 0, s_cam_rotXangle = 0 ) ;
      oscillator_position = Q2_origin ;   //  Initial position is center of grid.

      #ifdef GIF
        Q2_set( &oscillator_speed, 3072, -1536 ) ;
      #else
        oscillator_speed = Q2_origin ;   //  No initial speed.
      #endif
    break ;
  } ;

  grid_dist2osc_update( ) ;
}


void
oscillator_change
( )
{
  // Cycle trough the oscillator modes.
  switch (s_oscillator)
  {
    case OSCILLATOR_ANCHORED:
      oscillator_set( OSCILLATOR_FLOATING ) ;
    break ;

    case OSCILLATOR_FLOATING:
      oscillator_set( OSCILLATOR_BOUNCING ) ;
    break ;

    case OSCILLATOR_BOUNCING:
      oscillator_set( OSCILLATOR_ANCHORED ) ;
    break ;

    case OSCILLATOR_UNDEFINED:
      oscillator_set( OSCILLATOR_DEFAULT ) ;
    break ;
  }
}


void
oscillator_change_click_handler
( ClickRecognizerRef recognizer
, void              *context
)
{ oscillator_change( ) ; }


/***  ---------------  z := f( x, y )  ---------------  ***/

inline
static
Q
oscillator_distance
( const Q x, const Q y )
{
  const Q  dx = oscillator_position.x - x ;
  const Q  dy = oscillator_position.y - y ;

  return Q_sqrt( Q_mul( dx, dx ) + Q_mul( dy, dy ) ) ;
}


inline
static
Q
f_distance
( const Q dist )
{
  const int32_t angle = ((dist >> 1) + oscillator_anglePhase) & 0xFFFF ;   //  (distance2oscillator / 2 + anglePhase) % TRIG_MAX_RATIO
  return cos_lookup( angle ) ;                                             //  z = f( x, y )
}


inline
static
Q
f_XY
( const Q x, const Q y )
{ return f_distance( oscillator_distance( x, y ) ) ; }


/***  ---------------  Hidden line removal  ---------------  ***/

bool
function_isVisible_fromPoint
( const Q3     point
, const Q3     viewPoint
, const Boxing viewPointBoxing
)
{
  Q3 point2viewer ;  Q3_sub( &point2viewer, &viewPoint, &point ) ;
  Q  k ;

  //  1) Clip the view line to the nearest min/max box wall.
  if (viewPointBoxing.xMajor)
    k = Q_div( world_xMax - point.x, point2viewer.x ) ;
  else if (viewPointBoxing.xMinor)
    k = Q_div( world_xMin - point.x, point2viewer.x ) ;
  else
    k = Q_1 ;

  Q kMin = k ;

  if (viewPointBoxing.yMajor)
    k = Q_div( world_yMax - point.y, point2viewer.y ) ;
  else if (viewPointBoxing.yMinor)
    k = Q_div( world_yMin - point.y, point2viewer.y ) ;
  else
    k = Q_1 ;

  if (k < kMin)
    kMin = k ;

  if (viewPointBoxing.zMajor)
    k = Q_div( world_zMax - point.z, point2viewer.z ) ;
  else if (viewPointBoxing.zMinor)
    k = Q_div( world_zMin - point.z, point2viewer.z ) ;
  else
    k = Q_1 ;

  if (k < kMin)
    kMin = k ;

  // test for the point being epsilon close to the world box surface.
  if (kMin < (Q_1>>(VISIBILITY_MAX_ITERATIONS+1)))
    return true ;

  if (kMin < Q_1)
    Q3_sca( &point2viewer, kMin, &point2viewer ) ;    //  Do the clipping to the nearest min/max box wall.


  //  2) Test the clipped line segment with increasingly smaller steps.

  bool hasPositives = false ;
  bool hasNegatives = false ;

  Q3 smallStep, probe ;
  Q  smallStepK ;

  for ( smallStepK = Q_1     , smallStep = point2viewer                                  //  Start with the biggest possible small step, all the way to the nearest point in the min/max box (k=1).
      ; smallStepK >= Q_1>>VISIBILITY_MAX_ITERATIONS                                     //  Newton (split in half) steps. TODO: refine exit criteria.
      ; smallStepK >>= 1     , smallStep.x >>= 1, smallStep.y >>= 1, smallStep.z >>= 1   //  Divide the step length in half.
      )
  {
    Q3_add( &probe, &point, &smallStep ) ;

    Q3 bigStep ;
    Q  bigStepK ;

    for ( k = smallStepK,  bigStepK = smallStepK << 1,  bigStep.x = smallStep.x << 1,  bigStep.y = smallStep.y << 1,  bigStep.z = smallStep.z << 1
        ; k <= Q_1
        ; k += bigStepK ,  Q3_add( &probe, &probe, &bigStep )
        )
    {
      Q probeAltitude = probe.z - f_XY( probe.x, probe.y ) ;

      if (probeAltitude > Q_0)
      {
        if (hasNegatives)
          return false ;    // Not visible since it has both positive and negative probe altitudes (function altitude has zeros).

        hasPositives = true ;
      }
      else if (probeAltitude < Q_0)
      {
        if (hasPositives)
          return false ;    // Not visible since it has both positive and negative probe altitudes (function altitude has zeros).

        hasNegatives = true ;
      }
    }
  }

  return true ;
}


#ifdef GIF
  void world_update_timer_handler( void *data ) ;

  void
  gifStepper_advance_click_handler
  ( ClickRecognizerRef recognizer
  , void              *context
  )
  {
    if (s_world_updateTimer_ptr == NULL)
      s_world_updateTimer_ptr = app_timer_register( 0, world_update_timer_handler, NULL ) ;   // Schedule a world update.
  }
#else
  Sampler   *accelSampler_x = NULL ;    // To be allocated at accelSamplers_initialize( ).
  Sampler   *accelSampler_y = NULL ;    // To be allocated at accelSamplers_initialize( ).
  Sampler   *accelSampler_z = NULL ;    // To be allocated at accelSamplers_initialize( ).


  void
  accelSamplers_initialize
  ( )
  {
    accelSampler_x = Sampler_new( ACCEL_SAMPLER_CAPACITY ) ;
    accelSampler_y = Sampler_new( ACCEL_SAMPLER_CAPACITY ) ;
    accelSampler_z = Sampler_new( ACCEL_SAMPLER_CAPACITY ) ;

    for (int i = 0  ;  i < ACCEL_SAMPLER_CAPACITY  ;  ++i)
    {
      Sampler_push( accelSampler_x,  -81 ) ;   // STEADY viewPoint attractor.
      Sampler_push( accelSampler_y, -816 ) ;   // STEADY viewPoint attractor.
      Sampler_push( accelSampler_z, -571 ) ;   // STEADY viewPoint attractor.
    }
  }


  void
  accelSamplers_finalize
  ( )
  {
    free( Sampler_free( accelSampler_x ) ) ; accelSampler_x = NULL ;
    free( Sampler_free( accelSampler_y ) ) ; accelSampler_y = NULL ;
    free( Sampler_free( accelSampler_z ) ) ; accelSampler_z = NULL ;
  }


  // Acellerometer handlers.
  void
  accel_data_service_handler
  ( AccelData *data
  , uint32_t   num_samples
  )
  { }
#endif


/***  ---------------  oscillator---------  ***/

static Q   dy2[GRID_LINES] ;   // Auxiliary array.


void
grid_major_dist2osc_update
( )
{
  for (int j = 0  ;  j < GRID_LINES  ;  j++)
  {
    const Q dy = oscillator_position.y - (grid_major_y[j] << COORD_SHIFT) ;
    dy2[j] = Q_mul( dy, dy ) ;
  }

  for (int i = 0  ;  i < GRID_LINES  ;  i++)
  {
    const Q dx    = oscillator_position.x - (grid_major_x[i] << COORD_SHIFT) ;
    const Q dx2_i = Q_mul( dx, dx ) ;

    for (int j = 0  ;  j < GRID_LINES  ;  j++)
      grid_major_dist2osc[i][j] = Q_sqrt( dx2_i + dy2[j] ) >> DIST_SHIFT ;
  }
}


void
grid_minor_dist2osc_update
( )
{
  for (int j = 0  ;  j < GRID_LINES-1  ;  j++)
  {
    const Q dy = oscillator_position.y - (grid_minor_y[j] << COORD_SHIFT) ;
    dy2[j] = Q_mul( dy, dy ) ;
  }

  for (int i = 0  ;  i < GRID_LINES-1  ;  i++)
  {
    const Q dx    = oscillator_position.x - (grid_minor_x[i] << COORD_SHIFT) ;
    const Q dx2_i = Q_mul( dx, dx ) ;

    for (int j = 0  ;  j < GRID_LINES-1  ;  j++)
      grid_minor_dist2osc[i][j] = Q_sqrt( dx2_i + dy2[j] ) >> DIST_SHIFT ;
  }
}


void
grid_dist2osc_update
( )
{
  switch (s_pattern)
  {
    case PATTERN_DOTS:
    case PATTERN_STRIPES:
      grid_minor_dist2osc_update( ) ;

    case PATTERN_LINES:
    case PATTERN_GRID:
      grid_major_dist2osc_update( ) ;
    break ;

    case PATTERN_UNDEFINED:
    break ;
  }
}


void
grid_initialize
( )
{
  const Q distanceBetweenLines = Q_div( grid_scale, Q_from_int(GRID_LINES - 1) ) ;

  world_xMin = world_yMin = -grid_halfScale ;
  world_xMax = world_yMax = +grid_halfScale ;
  world_zMin = -Q_1 ;
  world_zMax = +Q_1 ;

  int l ;
  Q   lCoord ;

  for ( l = 0          , lCoord = -grid_halfScale
      ; l < GRID_LINES
      ; l++            , lCoord += distanceBetweenLines
      )
    grid_major_x[l] = grid_major_y[l] = lCoord >> COORD_SHIFT ;

  for ( l = 0          , lCoord = -grid_halfScale + (distanceBetweenLines >> 1)
      ; l < GRID_LINES-1
      ; l++            , lCoord += distanceBetweenLines
      )
    grid_minor_x[l] = grid_minor_y[l] = lCoord >> COORD_SHIFT ;
}


/***  ---------------  Color related  ---------------  ***/

static GColor     s_color_stroke ;
static GColor     s_color_background ;
static bool       s_color_isInverted ;


#ifdef PBL_COLOR
  static GColor  s_colorMap[8] ;

  void
  color_initialize
  ( )
  {
    s_color_stroke     = GColorWhite ;
    s_color_background = GColorBlack ;
    s_color_isInverted = false ;

    s_colorMap[7] = GColorWhite ;
    s_colorMap[6] = GColorMelon ;
    s_colorMap[5] = GColorMagenta ;
    s_colorMap[4] = GColorRed ;
    s_colorMap[3] = GColorCyan ;
    s_colorMap[2] = GColorYellow ;
    s_colorMap[1] = GColorGreen ;
    s_colorMap[0] = GColorVividCerulean ;
  }


  GColor
  Fuxel_color
  ( const Fuxel  *fPtr )
  {
    GColor color = s_color_background ;

    switch (s_colorization)
    {
      case COLORIZATION_UNDEFINED:
        color = s_color_background ;
      break ;

      case COLORIZATION_MONOCHROMATIC:
        color = s_color_stroke ;
      break ;

      case COLORIZATION_DISTANCE:
        color = s_colorMap[(fPtr->dist2osc >> 15) & 0b111] ;      //  (2 * distance) % 8
      break ;
    }

    switch (s_illumination)
    {
      case ILLUMINATION_UNDEFINED:
        return s_color_background ;

      case ILLUMINATION_DIFUSE:
        return color ;

      case ILLUMINATION_SPOTLIGHT:
        return fPtr->visibility.spotlight ? color : GColorDarkGray ;
    }

    return color ;   //  Will never reach this line, just to mute compiler error.
  }


  static bool   s_antialiasing = ANTIALIASING_DEFAULT ;

  void
  antialiasing_change_click_handler
  ( ClickRecognizerRef recognizer
  , void              *context
  )
  { s_antialiasing = !s_antialiasing ; }
#else
  void
  color_initialize
  ( )
  {
    s_color_stroke     = GColorBlack ;
    s_color_background = GColorWhite ;
    s_color_isInverted = true ;
  }


  ink_t
  Fuxel_ink
  ( const Fuxel  *fPtr )
  {
    ink_t ink = INK0 ;

    switch (s_colorization)
    {
      case COLORIZATION_MONOCHROMATIC:
      case COLORIZATION_UNDEFINED:
        ink = INK100 ;
      break ;

      case COLORIZATION_DISTANCE:
        ink = ((fPtr->dist2osc >> 15) & 0b1) ? INK50 : INK100 ;
      break ;
    }

    switch (s_illumination)
    {
      case ILLUMINATION_UNDEFINED:
        return INK0 ;

      case ILLUMINATION_DIFUSE:
        return ink ;

      case ILLUMINATION_SPOTLIGHT:
        return fPtr->visibility.spotlight ? ink : INK33 ;
    }

    return ink ;   //  Will never reach this line, just to mute compiler error.
  }


  void
  invert_change
  ( )
  {
    s_color_isInverted = !s_color_isInverted ;

    if (s_color_isInverted)
    {
      s_color_stroke     = GColorBlack ;
      s_color_background = GColorWhite ;
    }
    else
    {
      s_color_stroke     = GColorWhite ;
      s_color_background = GColorBlack ;
    }

    window_set_background_color          ( s_window          , s_color_background ) ;
    action_bar_layer_set_background_color( s_action_bar_layer, s_color_background ) ;
  }


  void
  invert_change_click_handler
  ( ClickRecognizerRef recognizer
  , void              *context
  )
  { invert_change( ) ; }
#endif


void
world_initialize
( )
{
  pattern_set( PATTERN_DEFAULT ) ;
  grid_initialize( ) ;
  color_initialize( ) ;

  colorization_set( COLORIZATION_DEFAULT ) ;
  illumination_set( ILLUMINATION_DEFAULT ) ;
  oscillator_set  ( OSCILLATOR_DEFAULT   ) ;
  transparency_set( TRANSPARENCY_DEFAULT ) ;

#ifndef GIF
  accelSamplers_initialize( ) ;
#endif

  cam_initialize( ) ;
  light_initialize( ) ;
}


// UPDATE WORLD OBJECTS PROPERTIES

void
grid_major_z_update
( )
{
  for (int i = 0  ;  i < GRID_LINES  ;  ++i)
    for (int j = 0  ;  j < GRID_LINES  ;  ++j)
      grid_major_z[i][j] = f_distance( grid_major_dist2osc[i][j] << DIST_SHIFT ) >> Z_SHIFT ;
}


void
grid_minor_z_update
( )
{
  for (int i = 0  ;  i < GRID_LINES-1  ;  i++)
    for (int j = 0  ;  j < GRID_LINES-1  ;  j++)
      grid_minor_z[i][j] = f_distance( grid_minor_dist2osc[i][j] << DIST_SHIFT ) >> Z_SHIFT ;
}


void
grid_z_update
( )
{
  switch (s_pattern)
  {
    case PATTERN_DOTS:
    case PATTERN_STRIPES:
      grid_minor_z_update( ) ;

    case PATTERN_LINES:
    case PATTERN_GRID:
      grid_major_z_update( ) ;
    break ;

    case PATTERN_UNDEFINED:
    break ;
  } ;
}


void
Visibility_set
( Visibility *visibilityPtr
, Q3          world
)
{
  visibilityPtr->cam = function_isVisible_fromPoint( world, s_cam.viewPoint, s_cam_viewPoint_boxing ) ;

  if (visibilityPtr->cam)
    switch (s_illumination)
    {
      case ILLUMINATION_UNDEFINED:
      case ILLUMINATION_DIFUSE:
        visibilityPtr->spotlight = false ;
      break ;

      case ILLUMINATION_SPOTLIGHT:
        visibilityPtr->spotlight = function_isVisible_fromPoint( world, s_spotlight, s_spotlight_boxing ) ;
      break ;
    }
  else
    visibilityPtr->spotlight = false ;
}


void
grid_major_visibility_update
( )
{
  switch (s_transparency)
  {
    case TRANSPARENCY_UNDEFINED:
    break ;

    case TRANSPARENCY_TRANSLUCENT:
      // Already set to true in transparency_set( )
    break ;

    case TRANSPARENCY_XRAY:
    case TRANSPARENCY_OPAQUE:
      for (int i = 0  ;  i < GRID_LINES  ;  ++i)
      {
        const Q  grid_major_x_i = grid_major_x[i] << COORD_SHIFT ;

        for (int j = 0  ;  j < GRID_LINES  ;  ++j)
        {
          Q3 world = (Q3){ .x = grid_major_x_i
                         , .y = grid_major_y[j] << COORD_SHIFT
                         , .z = grid_major_z[i][j] << Z_SHIFT
                         } ;

          Visibility_set( &grid_major_visibility[i][j], world ) ;
        }
      }
    break ;
  }
}


void
grid_minor_visibility_update
( )
{
  switch (s_transparency)
  {
    case TRANSPARENCY_UNDEFINED:
    break ;

    case TRANSPARENCY_TRANSLUCENT:
      // Already set to true in transparency_set( )
    break ;

    case TRANSPARENCY_XRAY:
    case TRANSPARENCY_OPAQUE:
      for (int i = 0  ;  i < GRID_LINES-1  ;  ++i)
      {
        const Q  grid_minor_x_i = grid_minor_x[i] << COORD_SHIFT ;

        for (int j = 0  ;  j < GRID_LINES-1  ;  ++j)
        {
          Q3 world = (Q3){ .x = grid_minor_x_i
                         , .y = grid_minor_y[j] << COORD_SHIFT
                         , .z = grid_minor_z[i][j] << Z_SHIFT
                         } ;

          Visibility_set( &grid_minor_visibility[i][j], world ) ;
        }
      }
    break ;
  }
}


void
grid_visibility_update
( )
{
  switch (s_pattern)
  {
    case PATTERN_DOTS:
    case PATTERN_STRIPES:
      grid_minor_visibility_update( ) ;

    case PATTERN_LINES:
    case PATTERN_GRID:
      grid_major_visibility_update( ) ;
    break ;

    case PATTERN_UNDEFINED:
    break ;
  } ;
}


void
position_setFromSensors
( Q2 *positionPtr )
{
  AccelData ad ;

  if (accel_service_peek( &ad ) < 0)         // Accel service not available.
    *positionPtr = Q2_origin ;
  else
  {
    positionPtr->x = Q_mul( grid_halfScale, ad.x << 6 ) ;
    positionPtr->y = Q_mul( grid_halfScale, ad.y << 6 ) ;
  }
}


void
acceleration_setFromSensors
( Q2 *accelerationPtr )
{
  AccelData ad ;

  if (accel_service_peek( &ad ) < 0)         // Accel service not available.
    *accelerationPtr = Q2_origin ;
  else
  {
    accelerationPtr->x = ad.x >> OSCILLATOR_INERTIA_LEVEL ;
    accelerationPtr->y = ad.y >> OSCILLATOR_INERTIA_LEVEL ;
  }
}


void
viewPoint_setFromSensors
( Q3 *viewPointPtr )
{
  #ifdef GIF
    // Fixed point view for GIF generation.
    Q3_set( viewPointPtr, Q_from_float( -0.1f ), Q_from_float( +1.0f ), Q_from_float( +0.7f ) ) ;
  #else
    {
      // Non GIF => Interactive: use acelerometer to affect camera's view point position.
      AccelData ad ;
  
      if (accel_service_peek( &ad ) < 0)         // Accel service not available.
      {
        Sampler_push( accelSampler_x,  -81 ) ;   // STEADY viewPoint attractor.
        Sampler_push( accelSampler_y, -816 ) ;   // STEADY viewPoint attractor.
        Sampler_push( accelSampler_z, -571 ) ;   // STEADY viewPoint attractor.
      }
      else
      {
        #ifdef EMU
          if (ad.x == 0  &&  ad.y == 0  &&  ad.z == -1000)   // Under EMU with SENSORS off this is the default output.
          {
            Sampler_push( accelSampler_x,  -81 ) ;
            Sampler_push( accelSampler_y, -816 ) ;
            Sampler_push( accelSampler_z, -571 ) ;
          }
          else                                               // If running under EMU the SENSOR feed must be ON.
          {
            Sampler_push( accelSampler_x, ad.x ) ;
            Sampler_push( accelSampler_y, ad.y ) ;
            Sampler_push( accelSampler_z, ad.z ) ;
          }
        #else
          Sampler_push( accelSampler_x, ad.x ) ;
          Sampler_push( accelSampler_y, ad.y ) ;
          Sampler_push( accelSampler_z, ad.z ) ;
        #endif
      }

      const float kAvg = 0.001f / accelSampler_x->samplesNum ;
      const float avgX = (float)(kAvg * accelSampler_x->samplesAcum ) ;
      const float avgY =-(float)(kAvg * accelSampler_y->samplesAcum ) ;
      const float avgZ =-(float)(kAvg * accelSampler_z->samplesAcum ) ;

      Q3_set( viewPointPtr, Q_from_float( avgX ), Q_from_float( avgY ), Q_from_float( avgZ ) ) ;
    }
  #endif
}


// UPDATE CAMERA

void
camera_update
( )
{
  switch (s_oscillator)
  {
    case OSCILLATOR_UNDEFINED:
    case OSCILLATOR_FLOATING:
    case OSCILLATOR_BOUNCING:
    break ;

    case OSCILLATOR_ANCHORED:
      viewPoint_setFromSensors( &s_cam_viewPoint ) ;
      s_cam_rotZangle += s_cam_rotZangleSpeed ;  s_cam_rotZangle &= 0xFFFF ;        // Keep angle normalized.
      s_cam_rotXangle += s_cam_rotXangleSpeed ;  s_cam_rotXangle &= 0xFFFF ;        // Keep angle normalized.
      cam_config( s_cam_viewPoint, s_cam_rotZangle, s_cam_rotXangle ) ;
    break ;
  }
}


void
oscillator_update
( )
{
  oscillator_anglePhase = TRIG_MAX_ANGLE - ((s_world_updateCount << 8) & 0xFFFF) ;   //  2*PI - (256 * s_world_updateCount) % TRIG_MAX_RATIO

  switch (s_oscillator)
  {
    case OSCILLATOR_ANCHORED:
      //  No need to call grid_dist2osc_update( ) because oscillator is not moving.
    break ;

    case OSCILLATOR_FLOATING:
      position_setFromSensors( &oscillator_position ) ;
      grid_dist2osc_update( ) ;
    break ;

    case OSCILLATOR_BOUNCING:
      #ifndef GIF
        //  1) set oscillator acceleration from sensor readings
        acceleration_setFromSensors( &oscillator_acceleration ) ;
  
        //  2) update oscillator speed with oscillator acceleration
        Q2_add( &oscillator_speed, &oscillator_speed, &oscillator_acceleration ) ;   //  oscillator_speed += oscillator_acceleration
      #endif

      //  3) affect oscillator position given current oscillator speed
      Q2_add( &oscillator_position, &oscillator_position, &oscillator_speed ) ;   //  oscillator_position += oscillator_speed

      //  4) detect boundary colisions
      //     clip position to stay inside grid boundaries.
      //     invert speed direction on colision for bounce effect

      if (oscillator_position.x < -grid_halfScale)
      {
        oscillator_position.x = -grid_halfScale ;
        oscillator_speed.x    = -oscillator_speed.x ;
      }
      else if (oscillator_position.x > grid_halfScale)
      {
        oscillator_position.x = grid_halfScale ;
        oscillator_speed.x    = -oscillator_speed.x ;
      }

      if (oscillator_position.y < -grid_halfScale)
      {
        oscillator_position.y = -grid_halfScale ;
        oscillator_speed.y    = -oscillator_speed.y ;
      }
      else if (oscillator_position.y > grid_halfScale)
      {
        oscillator_position.y = grid_halfScale ;
        oscillator_speed.y    = -oscillator_speed.y ;
      }

      grid_dist2osc_update( ) ;

      // 6) introduce some drag to dampen oscillator speed
      #ifndef GIF
        Q2 drag ;
        Q2_sub( &oscillator_speed, &oscillator_speed, Q2_sca( &drag, Q_1 >> OSCILLATOR_LUBRICATION_LEVEL, &oscillator_speed ) ) ;
      #endif
    break ;

    case OSCILLATOR_UNDEFINED:
    break ;
  }
}


void
world_update
( )
{
  ++s_world_updateCount ;   //   "Master clock" for everything.

  oscillator_update( ) ;
  grid_z_update( ) ;
  camera_update( ) ;
  grid_visibility_update( ) ;

  // this will queue a defered call to the world_draw( ) method.
  layer_mark_dirty( s_world_layer ) ;
}


void
function_draw_pixel
( GContext    *gCtx
, const Fuxel  f
)
{
#ifdef PBL_COLOR
  graphics_context_set_stroke_color( gCtx, f.pen.color ) ;
#endif

  graphics_draw_pixel( gCtx, f.screen ) ;
}


void
grid_major_drawPixel
( GContext *gCtx )
{
  for (int i = 0  ;  i < GRID_LINES  ;  ++i)
  {
    const Q   grid_major_x_i = grid_major_x[i] << COORD_SHIFT ;
    
    for (int j = 0  ;  j < GRID_LINES  ;  ++j)
    {
      Fuxel f = (Fuxel){ .world      = (Q3){ .x = grid_major_x_i
                                           , .y = grid_major_y[j] << COORD_SHIFT
                                           , .z = grid_major_z[i][j] << Z_SHIFT
                                           }
                       , .dist2osc   = grid_major_dist2osc[i][j] << DIST_SHIFT
                       , .visibility = grid_major_visibility[i][j]
                       , .screen     = grid_major_screen[i][j]
                       }
      ;

      if (f.visibility.cam)
      {
        #ifdef PBL_COLOR
          f.pen.color = Fuxel_color( &f ) ;
        #endif

        function_draw_pixel( gCtx, f ) ;
      }
    }
  }
}


void
grid_minor_drawPixel
( GContext *gCtx )
{
  for (int i = 0  ;  i < GRID_LINES-1  ;  i++)
  {
    const Q   grid_minor_x_i = grid_minor_x[i] << COORD_SHIFT ;
    
    for (int j = 0  ;  j < GRID_LINES-1 ;  j++)
    {
      Fuxel f = (Fuxel){ .world      = (Q3){ .x = grid_minor_x_i
                                           , .y = grid_minor_y[j] << COORD_SHIFT
                                           , .z = grid_minor_z[i][j] << Z_SHIFT
                                           }
                       , .dist2osc   = grid_minor_dist2osc[i][j] << DIST_SHIFT
                       , .visibility = grid_minor_visibility[i][j]
                       , .screen     = grid_minor_screen[i][j]
                       }
      ;

      if (f.visibility.cam)
      {
        #ifdef PBL_COLOR
          f.pen.color = Fuxel_color( &f ) ;
        #endif

        function_draw_pixel( gCtx, f ) ;
      }
    }
  }
}


void
grid_major_drawPixel_XRAY
( GContext *gCtx )
{
  for (int i = 0  ;  i < GRID_LINES  ;  ++i)
  {
    const Q   grid_major_x_i = grid_major_x[i] << COORD_SHIFT ;
    
    for (int j = 0  ;  j < GRID_LINES  ;  ++j)
    {
      Fuxel f = (Fuxel){ .world      = (Q3){ .x = grid_major_x_i
                                           , .y = grid_major_y[j] << COORD_SHIFT
                                           , .z = grid_major_z[i][j] << Z_SHIFT
                                           }
                       , .dist2osc   = grid_major_dist2osc[i][j] << DIST_SHIFT
                       , .visibility = grid_major_visibility[i][j]
                       , .screen     = grid_major_screen[i][j]
                       }
      ;

      if (!f.visibility.cam)
      {
        #ifdef PBL_COLOR
          f.pen.color = Fuxel_color( &f ) ;
        #endif

        function_draw_pixel( gCtx, f ) ;
      }
    }
  }
}


void
grid_minor_drawPixel_XRAY
( GContext *gCtx )
{
  for (int i = 0  ;  i < GRID_LINES-1  ;  ++i)
  {
    const Q   grid_minor_x_i = grid_minor_x[i] << COORD_SHIFT ;
    
    for (int j = 0  ;  j < GRID_LINES-1  ;  ++j)
    {
      Fuxel f = (Fuxel){ .world      = (Q3){ .x = grid_minor_x_i
                                           , .y = grid_minor_y[j] << COORD_SHIFT
                                           , .z = grid_minor_z[i][j] << Z_SHIFT
                                           }
                       , .dist2osc   = grid_minor_dist2osc[i][j] << DIST_SHIFT
                       , .visibility = grid_minor_visibility[i][j]
                       , .screen     = grid_minor_screen[i][j]
                       }
      ;

      if (!f.visibility.cam)
      {
        #ifdef PBL_COLOR
          f.pen.color = Fuxel_color( &f ) ;
        #endif

        function_draw_pixel( gCtx, f ) ;
      }
    }
  }
}


bool
Fuxel_visualyIdentical
( const Fuxel *f1Ptr
, const Fuxel *f2Ptr
)
{
  return f1Ptr->visibility.cam == f2Ptr->visibility.cam
  #ifdef PBL_COLOR
      && gcolor_equal( f1Ptr->pen.color, f2Ptr->pen.color )
  #else
      && f1Ptr->pen.ink == f2Ptr->pen.ink
  #endif
       ;
}


Fuxel
Fuxel_median
( const Fuxel *f1Ptr
, const Fuxel *f2Ptr
)
{
  Fuxel  median ;
    
  median.world.x  = (f1Ptr->world.x + f2Ptr->world.x) >> 1 ;
  median.world.y  = (f1Ptr->world.y + f2Ptr->world.y) >> 1 ;
  median.dist2osc = oscillator_distance( median.world.x, median.world.y ) ;
  median.world.z  = f_distance( median.dist2osc ) ;
  Visibility_set( &median.visibility, median.world ) ;
  screen_project( &median.screen, median.world ) ;

  #ifdef PBL_COLOR
    median.pen.color = Fuxel_color( &median ) ;
  #else
    median.pen.ink   = Fuxel_ink( &median ) ;
  #endif

  return median ;
}


void
function_draw_line
( GContext    *gCtx
, const Fuxel  f0
, const Fuxel  f1
)
{
  if (f0.visibility.cam || f1.visibility.cam)    //  One of the points is visible ?
  {
    if (!Fuxel_visualyIdentical( &f0, &f1 ))   //  Is there any cam/spotlight terminator to find ?
    {
      // Calculate screen distance between f0 & f1.
      int sdx = f0.screen.x - f1.screen.x  ;  if (sdx < 0) sdx = -sdx ;   // Abs delta screen x.
      int sdy = f0.screen.y - f1.screen.y  ;  if (sdy < 0) sdy = -sdy ;   // Abs delta screen y.
    
      if ((sdx + sdy) > TERMINATOR_TOLERANCE_PXL)   // Screen distance still too far apart ?
      {
        // Need to recursively zoom in on the visual terminator.
        Fuxel  half = Fuxel_median( &f0, &f1 ) ;
        function_draw_line( gCtx, f0, half ) ;
        function_draw_line( gCtx, half, f1 ) ;
        return ;
      }
    }
  
    // We reach this point because either there are no terminators between f0 & f1, or the screen distance is close enough to avoid needing to find them.
    #ifdef PBL_COLOR
      graphics_context_set_stroke_color( gCtx, f0.visibility.cam ? f0.pen.color : f1.pen.color ) ;
      graphics_draw_line( gCtx, f0.screen, f1.screen ) ;
    #else
      Draw2D_line_pattern( gCtx, f0.screen.x, f0.screen.y, f1.screen.x, f1.screen.y, f0.visibility.cam ? f0.pen.ink : f1.pen.ink ) ;
    #endif
  }
}


// x parallel line form function point (Fuxel) f0 to f1.
void
grid_major_drawLineX
( GContext *gCtx
, int       j
)
{
  Fuxel f0, f1 ;
  const Q grid_major_y_j = grid_major_y[j] << COORD_SHIFT ;

  f1 = (Fuxel){ .world      = (Q3){ .x = grid_major_x[0] << COORD_SHIFT
                                  , .y = grid_major_y_j
                                  , .z = grid_major_z[0][j] << Z_SHIFT
                                  }
              , .dist2osc   = grid_major_dist2osc[0][j] << DIST_SHIFT
              , .visibility = grid_major_visibility[0][j]
              , .screen     = grid_major_screen[0][j]
              }
  ;

  #ifdef PBL_COLOR
    f1.pen.color = Fuxel_color( &f1 ) ;
  #else
    f1.pen.ink   = Fuxel_ink( &f1 ) ;
  #endif

  for (int i = 1  ;  i < GRID_LINES ;  ++i)
  {
    f0 = f1 ;

    f1 = (Fuxel){ .world      =  (Q3){ .x = grid_major_x[i] << COORD_SHIFT
                                     , .y = grid_major_y_j
                                     , .z = grid_major_z[i][j] << Z_SHIFT
                                     }
                , .dist2osc   = grid_major_dist2osc[i][j] << DIST_SHIFT
                , .visibility = grid_major_visibility[i][j]
                , .screen     = grid_major_screen[i][j]
                }
    ;

    #ifdef PBL_COLOR
      f1.pen.color = Fuxel_color( &f1 ) ;
    #else
      f1.pen.ink   = Fuxel_ink( &f1 ) ;
    #endif

    #ifdef GIF
      Fuxel  half = Fuxel_median( &f0, &f1 ) ;
      function_draw_line( gCtx, f0, half ) ;
      function_draw_line( gCtx, half, f1 ) ;
    #else
      function_draw_line( gCtx, f0, f1 ) ;
    #endif
  }
}


// y parallel line.
void
grid_major_drawLineY
( GContext *gCtx
, int       i
)
{
  Fuxel f0, f1 ;
  const Q grid_major_x_i = grid_major_x[i] << COORD_SHIFT ;

  f1 = (Fuxel){ .world      = (Q3){ .x = grid_major_x_i
                                  , .y = grid_major_y[0] << COORD_SHIFT
                                  , .z = grid_major_z[i][0] << Z_SHIFT
                                  }
              , .visibility = grid_major_visibility[i][0]
              , .dist2osc   = grid_major_dist2osc[i][0] << DIST_SHIFT
              , .screen     = grid_major_screen[i][0]
              }
  ;

  #ifdef PBL_COLOR
    f1.pen.color = Fuxel_color( &f1 ) ;
  #else
    f1.pen.ink   = Fuxel_ink( &f1 ) ;
  #endif

  for (int j = 1  ;  j < GRID_LINES ;  ++j)
  {
    f0 = f1 ;

    f1 = (Fuxel){ .world      = (Q3){ .x = grid_major_x_i
                                    , .y = grid_major_y[j] << COORD_SHIFT
                                    , .z = grid_major_z[i][j] << Z_SHIFT
                                    }
                , .visibility = grid_major_visibility[i][j]
                , .dist2osc   = grid_major_dist2osc[i][j] << DIST_SHIFT
                , .screen     = grid_major_screen[i][j]
                }
    ;

    #ifdef PBL_COLOR
      f1.pen.color = Fuxel_color( &f1 ) ;
    #else
      f1.pen.ink   = Fuxel_ink( &f1 ) ;
    #endif

    #ifdef GIF
      Fuxel  half = Fuxel_median( &f0, &f1 ) ;
      function_draw_line( gCtx, f0, half ) ;
      function_draw_line( gCtx, half, f1 ) ;
    #else
      function_draw_line( gCtx, f0, f1 ) ;
    #endif
  }
}


void
grid_major_drawLinesX
( GContext *gCtx )
{
  for (int l = 0  ;  l < GRID_LINES  ;  ++l)
    grid_major_drawLineX( gCtx, l ) ;
}


void
grid_major_drawLinesY
( GContext *gCtx )
{
  for (int l = 0  ;  l < GRID_LINES  ;  ++l)
    grid_major_drawLineY( gCtx, l ) ;
}


void
grid_major_screen_project
( )
{
  for (int i = 0  ;  i < GRID_LINES  ;  ++i)
  {
    const Q grid_major_x_i = grid_major_x[i] << COORD_SHIFT ;

    for (int j = 0  ;  j < GRID_LINES  ;  ++j)
      screen_project( &grid_major_screen[i][j]
                    , (Q3){ .x = grid_major_x_i
                          , .y = grid_major_y[j] << COORD_SHIFT
                          , .z = grid_major_z[i][j] << Z_SHIFT
                          }
                    ) ;
  }
}


void
grid_minor_drawLineX
( GContext *gCtx
, int       j
)
{
  Fuxel f0, f1 ;
  const Q grid_minor_y_j = grid_minor_y[j] << COORD_SHIFT ;

  f1 = (Fuxel){ .world      = (Q3){ .x = grid_minor_x[0] << COORD_SHIFT
                                  , .y = grid_minor_y_j
                                  , .z = grid_minor_z[0][j] << Z_SHIFT
                                  }
              , .dist2osc   = grid_minor_dist2osc[0][j] << DIST_SHIFT
              , .visibility = grid_minor_visibility[0][j]
              , .screen     = grid_minor_screen[0][j]
              }
  ;

  #ifdef PBL_COLOR
    f1.pen.color = Fuxel_color( &f1 ) ;
  #else
    f1.pen.ink   = Fuxel_ink( &f1 ) ;
  #endif

  for (int i = 1  ;  i < GRID_LINES-1 ;  ++i)
  {
    f0 = f1 ;
    
    f1 = (Fuxel){ .world      = (Q3){ .x = grid_minor_x[i] << COORD_SHIFT
                                    , .y = grid_minor_y_j
                                    , .z = grid_minor_z[i][j] << Z_SHIFT
                                    }
                , .dist2osc   = grid_minor_dist2osc[i][j] << DIST_SHIFT
                , .visibility = grid_minor_visibility[i][j]
                , .screen     = grid_minor_screen[i][j]
                }
    ;

    #ifdef PBL_COLOR
      f1.pen.color = Fuxel_color( &f1 ) ;
    #else
      f1.pen.ink   = Fuxel_ink( &f1 ) ;
    #endif

    #ifdef GIF
      Fuxel  half = Fuxel_median( &f0, &f1 ) ;
      function_draw_line( gCtx, f0, half ) ;
      function_draw_line( gCtx, half, f1 ) ;
    #else
      function_draw_line( gCtx, f0, f1 ) ;
    #endif
  }
}


void
grid_minor_drawLinesX
( GContext *gCtx )
{
  for (int l = 0  ;  l < GRID_LINES-1  ;  ++l)
    grid_minor_drawLineX( gCtx, l ) ;
}


void
grid_minor_screen_project
( )
{
  for (int i = 0  ;  i < GRID_LINES-1  ;  ++i)
  {
    const Q grid_minor_x_i = grid_minor_x[i] << COORD_SHIFT ;

    for (int j = 0  ;  j < GRID_LINES-1  ;  ++j)
      screen_project( &grid_minor_screen[i][j]
                    , (Q3){ .x = grid_minor_x_i
                          , .y = grid_minor_y[j] << COORD_SHIFT
                          , .z = grid_minor_z[i][j] << Z_SHIFT
                          }
                    ) ;
  }
}


void
grid_screen_project
( )
{
  switch (s_pattern)
  {
    case PATTERN_UNDEFINED:
    break ;

    case PATTERN_DOTS:
    case PATTERN_STRIPES:
      grid_minor_screen_project( ) ;

    case PATTERN_LINES:
    case PATTERN_GRID:
      grid_major_screen_project( ) ;
    break ;
  }
}


void
world_draw
( Layer    *me
, GContext *gCtx
)
{
#ifdef GIF
  LOGD( "world_draw:: s_world_updateCount = %d", s_world_updateCount ) ;
#endif

#ifdef PBL_COLOR
  graphics_context_set_antialiased( gCtx, s_antialiasing ) ;
#else
  graphics_context_set_stroke_color( gCtx, s_color_stroke ) ;
#endif

  grid_screen_project( ) ;

  // Draw the calculated screen points.
  switch (s_pattern)
  {
    case PATTERN_UNDEFINED:
    break ;

    case PATTERN_DOTS:
      if (s_transparency == TRANSPARENCY_XRAY)
      {
        grid_major_drawPixel_XRAY( gCtx ) ;
        grid_minor_drawPixel_XRAY( gCtx ) ;
      }

      grid_major_drawPixel( gCtx ) ;
      grid_minor_drawPixel( gCtx ) ;

      // Grid frame.
      grid_major_drawLineX( gCtx, 0            ) ;
      grid_major_drawLineX( gCtx, GRID_LINES-1 ) ;
      grid_major_drawLineY( gCtx, 0            ) ;
      grid_major_drawLineY( gCtx, GRID_LINES-1 ) ;
    break ;

    case PATTERN_LINES:
      if (s_transparency == TRANSPARENCY_XRAY)
        grid_major_drawPixel_XRAY( gCtx ) ;

      grid_major_drawLinesX( gCtx ) ;

      // Grid frame.
      grid_major_drawLineY( gCtx, 0            ) ;
      grid_major_drawLineY( gCtx, GRID_LINES-1 ) ;
    break ;

    case PATTERN_STRIPES:
      if (s_transparency == TRANSPARENCY_XRAY)
      {
        grid_major_drawPixel_XRAY( gCtx ) ;
        grid_minor_drawPixel_XRAY( gCtx ) ;
      }

      grid_major_drawLinesX( gCtx ) ;
      grid_minor_drawLinesX( gCtx ) ;

      // Grid frame.
      grid_major_drawLineY( gCtx, 0            ) ;
      grid_major_drawLineY( gCtx, GRID_LINES-1 ) ;
    break ;

    case PATTERN_GRID:
      if (s_transparency == TRANSPARENCY_XRAY)
        grid_major_drawPixel_XRAY( gCtx ) ;

      grid_major_drawLinesX( gCtx ) ;
      grid_major_drawLinesY( gCtx ) ;
    break ;
  }
}


void
world_finalize
( )
{
#ifndef GIF
  accelSamplers_finalize( ) ;
#endif
}


void
world_update_timer_handler
( void *data )
{
  s_world_updateTimer_ptr = NULL ;
  world_update( ) ;

#ifdef GIF
  if (s_world_updateCount < GIF_STOP_COUNT)
#endif
  s_world_updateTimer_ptr = app_timer_register( ANIMATION_INTERVAL_MS, world_update_timer_handler, data ) ;
}


void
world_start
( )
{
#ifndef GIF
  // Gravity aware.
 	accel_data_service_subscribe( 0, accel_data_service_handler ) ;
#endif

  // Start animation.
  world_update_timer_handler( NULL ) ;
}


void
world_stop
( )
{
  // Stop animation.
  if (s_world_updateTimer_ptr != NULL)
  {
    app_timer_cancel( s_world_updateTimer_ptr ) ;
    s_world_updateTimer_ptr = NULL ;
  }

#ifndef GIF
  // Gravity unaware.
  accel_data_service_unsubscribe( ) ;
#endif
}


void
click_config_provider
( void *context )
{
  // Single click.
  window_single_click_subscribe( BUTTON_ID_UP
                               , (ClickHandler) colorization_change_click_handler  
                               ) ;

  window_single_click_subscribe( BUTTON_ID_SELECT
                               , (ClickHandler) pattern_change_click_handler
                               ) ;

#ifndef GIF
  window_single_click_subscribe( BUTTON_ID_DOWN
                               , (ClickHandler) oscillator_change_click_handler
                               ) ;
#else
  window_single_click_subscribe( BUTTON_ID_DOWN
                               , (ClickHandler) gifStepper_advance_click_handler
                               ) ;
#endif

  // Long click.
  window_long_click_subscribe( BUTTON_ID_UP
                             , 0
                             , (ClickHandler) illumination_change_click_handler
                             , NULL
                             ) ;

  window_long_click_subscribe( BUTTON_ID_SELECT
                             , 0
                             , (ClickHandler) transparency_change_click_handler
                             , NULL
                             ) ;

#ifdef PBL_COLOR
  window_long_click_subscribe( BUTTON_ID_DOWN
                             , 0
                             , (ClickHandler) antialiasing_change_click_handler
                             , NULL
                             ) ;
#else
  window_long_click_subscribe( BUTTON_ID_DOWN
                             , 0
                             , (ClickHandler) invert_change_click_handler
                             , NULL
                             ) ;
#endif
}



void
window_load
( Window *window )
{
  // Create and configure the layers
        s_window_layer        = window_get_root_layer( window ) ;
  GSize screen_availableSize  = layer_get_unobstructed_bounds( s_window_layer ).size ;

  screen_project_scale = (screen_availableSize.w > screen_availableSize.h)
                       ? Q_from_int(screen_availableSize.h)
                       : Q_from_int(screen_availableSize.w)
                       ;

  screen_project_translate.x = Q_from_int(screen_availableSize.w) >> 1 ;
  screen_project_translate.y = Q_from_int(screen_availableSize.h) >> 1 ;

  s_action_bar_layer = action_bar_layer_create( ) ;
  action_bar_layer_set_background_color     ( s_action_bar_layer, s_color_background    ) ;
  action_bar_layer_set_click_config_provider( s_action_bar_layer, click_config_provider ) ;

  s_world_layer = layer_create( layer_get_frame( s_window_layer ) ) ;
  layer_set_update_proc( s_world_layer, world_draw ) ;

  // Add the layers to the main window layer.
  action_bar_layer_add_to_window( s_action_bar_layer, s_window ) ;
  layer_add_child( s_window_layer, s_world_layer ) ;

  world_start( ) ;
}


void
window_unload
( Window *window )
{
  world_stop( ) ;

  // Destroy layers.
  layer_destroy( s_world_layer ) ;
}


void
app_initialize
( void )
{
  world_initialize( ) ;

  s_window = window_create( ) ;
  window_set_background_color( s_window, s_color_background ) ;

  window_set_window_handlers( s_window
                            , (WindowHandlers)
                              { .load   = window_load
                              , .unload = window_unload
                              }
                            ) ;

  window_stack_push( s_window, false ) ;
}


void
app_finalize
( void )
{
  window_stack_remove( s_window, false ) ;
  window_destroy( s_window ) ;
  world_finalize( ) ;
}


int
main
( void )
{
  app_initialize( ) ;
  app_event_loop( ) ;
  app_finalize( ) ;
}