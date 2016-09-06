/*
   WatchApp: Ripples 3D
   File    : main.c
   Author  : Afonso Santos, Portugal
   Notes   : Dedicated to all the @PebbleDev team and to @KatharineBerry in particular
           : ... for her CloudPebble online dev environment that made this possible.

   Last revision: 16h25 September 06 2016  GMT
*/

#include <pebble.h>
#include <karambola/Q3.h>
#include <karambola/CamQ3.h>
#include <karambola/Sampler.h>

#ifndef PBL_COLOR
  #include <karambola/Draw2D.h>
#endif

#include "main.h"
#include "Config.h"


// UI related
static Window         *s_window ;
static Layer          *s_window_layer ;
static Layer          *s_world_layer ;
static ActionBarLayer *s_action_bar_layer ;

// Obstruction related.
static GSize available_screen ;


// World related
Q3     worldPoints [GRID_LINES][GRID_LINES] ;
Q      dist_xy     [GRID_LINES][GRID_LINES] ;
GPoint screenPoints[GRID_LINES][GRID_LINES] ;

const Q  grid_scale = Q_from_float(GRID_SCALE) ;

static int        s_world_updateCount       = 0 ;
static AppTimer  *s_world_updateTimer_ptr   = NULL ;


typedef enum { RENDER_MODE_UNDEFINED
             , RENDER_MODE_DOTS
             , RENDER_MODE_LINES
             }
RenderMode ;

static RenderMode    s_render_mode = RENDER_MODE_DEFAULT ;

void
renderMode_change_click_handler
( ClickRecognizerRef recognizer
, void              *context
)
{
  // Cycle trough the render modes.
  switch (s_render_mode)
  {
    case RENDER_MODE_DOTS:
     s_render_mode = RENDER_MODE_LINES ;
     break ;

   case RENDER_MODE_LINES:
     s_render_mode = RENDER_MODE_DOTS ;
     break ;

   default:
     s_render_mode = RENDER_MODE_DEFAULT ;
     break ;
  } ;
}


// Camera related

static CamQ3   s_cam ;
static Q       s_cam_zoom        = PBL_IF_RECT_ELSE(Q_from_float(+1.25f), Q_from_float(+1.15f)) ;
static int32_t s_cam_rotZangle   = 0 ;
static int32_t s_cam_rotZangleStep ;


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
Sampler   *sampler_accelX = NULL ;    // To be allocated at sampler_initialize( ).
Sampler   *sampler_accelY = NULL ;    // To be allocated at sampler_initialize( ).
Sampler   *sampler_accelZ = NULL ;    // To be allocated at sampler_initialize( ).


void
sampler_initialize
( )
{
  sampler_accelX = Sampler_new( ACCEL_SAMPLER_CAPACITY ) ;
  sampler_accelY = Sampler_new( ACCEL_SAMPLER_CAPACITY ) ;
  sampler_accelZ = Sampler_new( ACCEL_SAMPLER_CAPACITY ) ;

  for ( int i = 0  ;  i < ACCEL_SAMPLER_CAPACITY  ;  ++i )
  {
    Sampler_push( sampler_accelX,  -81 ) ;   // STEADY viewPoint attractor.
    Sampler_push( sampler_accelY, -816 ) ;   // STEADY viewPoint attractor.
    Sampler_push( sampler_accelZ, -571 ) ;   // STEADY viewPoint attractor.
  }
}


void
sampler_finalize
( )
{
  free( Sampler_free( sampler_accelX ) ) ; sampler_accelX = NULL ;
  free( Sampler_free( sampler_accelY ) ) ; sampler_accelY = NULL ;
  free( Sampler_free( sampler_accelZ ) ) ; sampler_accelZ = NULL ;
}


// Acellerometer handlers.
void
accel_data_service_handler
( AccelData *data
, uint32_t   num_samples
)
{ }
#endif


void
cam_config
( const Q3      *pViewPoint
, const int32_t  pRotZangle
)
{
  Q3 scaledVP ;
  Q3_scaTo( &scaledVP, Q_from_float(CAM3D_DISTANCEFROMORIGIN), pViewPoint ) ;

  Q3 rotatedVP ;
  Q3_rotZ( &rotatedVP, &scaledVP, pRotZangle ) ;

  // setup 3D camera
  CamQ3_lookAtOriginUpwards( &s_cam, &rotatedVP, s_cam_zoom, CAM_PROJECTION_PERSPECTIVE ) ;
}


void
function_initialize
( )
{
  const Q halfScale         = grid_scale >> 1 ;   // scale / 2
  const Q gridXY_numStripes = Q_from_int(GRID_LINES - 1) ;
  const Q gridStep          = Q_div( grid_scale, gridXY_numStripes ) ;

  int i ;
  Q   x ;

  for ( i = 0          , x = -halfScale
      ; i < GRID_LINES
      ; i++            , x += gridStep
      )
  {
    const Q x2 = Q_mul(x, x) ;

    int j ;
    Q   y ;

    for ( j = 0          , y = -halfScale
        ; j < GRID_LINES
        ; j++            , y += gridStep
        )
    {
      worldPoints[i][j].x = x ;
      worldPoints[i][j].y = y ;

      const Q y2 = Q_mul(y, y) ;
      dist_xy[i][j] = Q_sqrt( x2 + y2 ) ;
    }
  }
}


#ifdef PBL_COLOR
typedef enum { COLOR_MODE_MONO
             , COLOR_MODE_SIGNAL
             , COLOR_MODE_HEIGHT
             , COLOR_MODE_DIST
             }
ColorMode ;
#else
typedef enum { COLOR_MODE_MONO
             , COLOR_MODE_SIGNAL
             , COLOR_MODE_DIST
             }
ColorMode ;
#endif


static bool       s_isInverted ;
static GColor     s_background_color ;
static GColor     s_stroke_color ;
static ColorMode  s_color_mode = COLOR_MODE_DEFAULT ;


void
colorMode_change_click_handler
( ClickRecognizerRef recognizer
, void              *context
)
{
#ifdef PBL_COLOR
  // Cycle trough the color modes.
  switch (s_color_mode)
  {
    case COLOR_MODE_MONO:
     s_color_mode = COLOR_MODE_SIGNAL ;
     break ;

    case COLOR_MODE_SIGNAL:
     s_color_mode = COLOR_MODE_DIST ;
     break ;

   case COLOR_MODE_DIST:
     s_color_mode = COLOR_MODE_HEIGHT ;
     break ;

   case COLOR_MODE_HEIGHT:
     s_color_mode = COLOR_MODE_MONO ;
     break ;

   default:
     s_color_mode = RENDER_MODE_DEFAULT ;
     break ;
  } ;
#else
  // Cycle trough the color modes.
  switch (s_color_mode)
  {
    case COLOR_MODE_MONO:
     s_color_mode = COLOR_MODE_SIGNAL ;
     break ;

    case COLOR_MODE_SIGNAL:
     s_color_mode = COLOR_MODE_DIST ;
     break ;

   case COLOR_MODE_DIST:
     s_color_mode = COLOR_MODE_MONO ;
     break ;

   default:
     s_color_mode = RENDER_MODE_DEFAULT ;
     break ;
  } ;
#endif
}


#ifdef PBL_COLOR
static GColor  s_colorMap[8] ;

void
color_initialize
( )
{
  s_stroke_color     = GColorWhite ;
  s_background_color = GColorBlack ;
  s_isInverted       = false ;
  
  s_colorMap[7] = GColorWhite ;
  s_colorMap[6] = GColorMelon ;
  s_colorMap[5] = GColorMagenta ;
  s_colorMap[4] = GColorRed ;
  s_colorMap[3] = GColorCyan ;
  s_colorMap[2] = GColorYellow ;
  s_colorMap[1] = GColorGreen ;
  s_colorMap[0] = GColorVividCerulean ;
}


void
set_stroke_color
( GContext *gCtx
, const int i
, const int j
)
{
  switch (s_color_mode)
  {
    case COLOR_MODE_SIGNAL:
      if (worldPoints[i][j].z > Q_0)
        graphics_context_set_stroke_color( gCtx, GColorMelon ) ;           //  z > 0
      else
        graphics_context_set_stroke_color( gCtx, GColorVividCerulean ) ;   //  z <= 0
    break ;

    case COLOR_MODE_DIST:
      graphics_context_set_stroke_color( gCtx, s_colorMap[((dist_xy[i][j]) >> 15) & 0b111] ) ;               //  (dist * 2) % 8
    break ;

    case COLOR_MODE_HEIGHT:
      graphics_context_set_stroke_color( gCtx, s_colorMap[((worldPoints[i][j].z + Q_1) >> 14) & 0b111] ) ;   // ((z + 1) * 4) % 8
    break ;

    case COLOR_MODE_MONO:
    default:
      graphics_context_set_stroke_color( gCtx, s_stroke_color ) ;
    break ;
  } ;
}


static bool   s_antialiasing = ANTIALIASING_DEFAULT ;

void
antialiasing_change_click_handler
( ClickRecognizerRef recognizer
, void              *context
)
{
  s_antialiasing = !s_antialiasing ;
}
#else
void
color_initialize
( )
{
  s_stroke_color     = GColorBlack ;
  s_background_color = GColorWhite ;
  s_isInverted       = true ;
}


ink_t
get_stroke_ink
( const int i
, const int j
)
{
  switch (s_color_mode)
  {
    case COLOR_MODE_MONO:
      return INK100 ;
    break ;
    
    case COLOR_MODE_SIGNAL:
      return (worldPoints[i][j].z > Q_0)  ?  INK100  :  INK33 ;
    break ;

    case COLOR_MODE_DIST:
      switch (((dist_xy[i][j]) >> 15) & 0b1 )
      {
        case 1:
          return INK33 ;

        case 0:
        default:
          return INK100 ;
        break ;
      }
    break ;

    default:
      return INK100 ;
    break ;
  } ;
}


void
invert_change_click_handler
( ClickRecognizerRef recognizer
, void              *context
)
{
  s_isInverted = !s_isInverted ;

  if (s_isInverted)
  {
    s_stroke_color     = GColorBlack ;
    s_background_color = GColorWhite ;
  }
  else
  {
    s_stroke_color     = GColorWhite ;
    s_background_color = GColorBlack ;
  }

  window_set_background_color( s_window, s_background_color ) ;
  action_bar_layer_set_background_color( s_action_bar_layer, s_background_color) ;
}
#endif


void
world_initialize
( )
{
  function_initialize( ) ;

#ifndef GIF
  sampler_initialize( ) ;
#endif

  // Initialize cam rotation vars.
  s_cam_rotZangle     = 0 ;
  s_cam_rotZangleStep = TRIG_MAX_ANGLE >> 9 ;  //  2 * PI / 512

  color_initialize( ) ;
}


// UPDATE CAMERA & WORLD OBJECTS PROPERTIES

void
function_update
( )
{
  // Update z = f( x, y )
  int32_t anglePhase = TRIG_MAX_ANGLE - ((s_world_updateCount << 8) & 0xFFFF) ;   // s_world_updateCount * 256 % TRIG_MAX_RATIO

  for (int i = 0  ;  i < GRID_LINES  ;  ++i)
    for (int j = 0  ;  j < GRID_LINES  ;  ++j)
    {
      int32_t angle = ((dist_xy[i][j] >> 1) + anglePhase) & 0xFFFF ;   // (dist_xy[i][j] / 2 + anglePhase) % TRIG_MAX_RATIO
      worldPoints[i][j].z = cos_lookup( angle ) ;
    }
}


void
camera_update
( )
{
  static Q3 viewPoint ;

#ifdef GIF
  viewPoint.x = Q_from_float( -0.1f ) ;
  viewPoint.y = Q_from_float( +1.0f ) ;
  viewPoint.z = Q_from_float( +0.7f ) ;
#else
  // Use acelerometer to affect camera's view point position.
  AccelData ad ;

  if (accel_service_peek( &ad ) < 0)         // Accel service not available.
  {
    Sampler_push( sampler_accelX,  -81 ) ;   // STEADY viewPoint attractor. 
    Sampler_push( sampler_accelY, -816 ) ;   // STEADY viewPoint attractor.
    Sampler_push( sampler_accelZ, -571 ) ;   // STEADY viewPoint attractor.
  }
  else
  {
  #ifdef EMU
    if (ad.x == 0  &&  ad.y == 0  &&  ad.z == -1000)   // Under EMU with SENSORS off this is the default output.
    {
      Sampler_push( sampler_accelX,  -81 ) ;
      Sampler_push( sampler_accelY, -816 ) ;
      Sampler_push( sampler_accelZ, -571 ) ;
    }
    else                                               // If running under EMU the SENSOR feed must be ON.
    {
      Sampler_push( sampler_accelX, ad.x ) ;
      Sampler_push( sampler_accelY, ad.y ) ;
      Sampler_push( sampler_accelZ, ad.z ) ;
    }
  #else
    Sampler_push( sampler_accelX, ad.x ) ;
    Sampler_push( sampler_accelY, ad.y ) ;
    Sampler_push( sampler_accelZ, ad.z ) ;
  #endif
  }

  const float kAvg = 0.001f / sampler_accelX->samplesNum ;
  const float avgX = (float)(kAvg * sampler_accelX->samplesAcum ) ;
  const float avgY =-(float)(kAvg * sampler_accelY->samplesAcum ) ;
  const float avgZ =-(float)(kAvg * sampler_accelZ->samplesAcum ) ;

  viewPoint.x = Q_from_float( avgX ) ;
  viewPoint.y = Q_from_float( avgY ) ;
  viewPoint.z = Q_from_float( avgZ ) ;
#endif
      
  s_cam_rotZangle += s_cam_rotZangleStep ;
  s_cam_rotZangle &= 0xFFFF ;        // Keep angle normalized.
  cam_config( &viewPoint, s_cam_rotZangle ) ;
}


void
world_update
( )
{
  ++s_world_updateCount ;
  
  function_update( ) ;
  camera_update( ) ;

  // this will queue a defered call to the world_draw( ) method.
  layer_mark_dirty( s_world_layer ) ;
}


void
world_draw
( Layer    *me
, GContext *gCtx
)
{
  LOGD( "world_draw:: s_world_updateCount = %d", s_world_updateCount ) ;

#ifdef PBL_COLOR
  graphics_context_set_antialiased( gCtx, s_antialiasing ) ;
#else
  graphics_context_set_stroke_color( gCtx, s_stroke_color ) ;
#endif

  // Draw f(x,y)
  const Q  k = (available_screen.w > available_screen.h)
             ? Q_from_int(available_screen.h)
             : Q_from_int(available_screen.w)
             ;

  const Q  bX = Q_from_int(available_screen.w) >> 1 ;   // bX = available_screen.w / 2
  const Q  bY = Q_from_int(available_screen.h) >> 1 ;   // bY = available_screen.h / 2

  // Calculate screen points.
  for (int i = 0  ;  i < GRID_LINES  ;  ++i)
    for (int j = 0  ;  j < GRID_LINES  ;  ++j)
    {
      // Calculate camera film plane 2D coordinates of 3D world points.
      Q2 vCamera ;
      CamQ3_view( &vCamera, &s_cam, &worldPoints[i][j] ) ;
  
      // Convert camera coordinates to screen/device coordinates.
      Q x = Q_mul( k, vCamera.x )  +  bX ;
      Q y = Q_mul( k, vCamera.y )  +  bY ;
      screenPoints[i][j].x = Q_to_int(x) ;
      screenPoints[i][j].y = Q_to_int(y) ;
    }

  // Draw the calculated screen points.
  switch (s_render_mode)
  {
    case RENDER_MODE_DOTS:
      for (int i = 0  ;  i < GRID_LINES  ;  ++i)
        for (int j = 0  ;  j < GRID_LINES  ;  ++j)
        {
#ifdef PBL_COLOR
          set_stroke_color( gCtx, i, j ) ;
#endif
          graphics_draw_pixel( gCtx, screenPoints[i][j] ) ;
        }
    break ;   //  case RENDER_MODE_DOTS:

    case RENDER_MODE_LINES:
      // y parallel lines.
      for (int i = 0  ;  i < GRID_LINES  ;  ++i)
        for (int j = 1  ;  j < GRID_LINES  ;  ++j)
        {
#ifdef PBL_COLOR
          set_stroke_color( gCtx, i, j ) ;
          graphics_draw_line( gCtx, screenPoints[i][j], screenPoints[i][j-1] ) ;
#else
          GPoint *p0 = &screenPoints[i][j] ;
          GPoint *p1 = &screenPoints[i][j-1] ;
          Draw2D_line_pattern( gCtx, p0->x, p0->y, p1->x, p1->y, get_stroke_ink( i, j ) ) ;
#endif
        }

      // x parallel lines.
      for (int j = 0  ;  j < GRID_LINES  ;  ++j)
        for (int i = 1  ;  i < GRID_LINES  ;  ++i)
        {
#ifdef PBL_COLOR
          set_stroke_color( gCtx, i, j ) ;
          graphics_draw_line( gCtx, screenPoints[i][j], screenPoints[i-1][j] ) ;
#else
          GPoint *p0 = &screenPoints[i][j] ;
          GPoint *p1 = &screenPoints[i-1][j] ;
          Draw2D_line_pattern( gCtx, p0->x, p0->y, p1->x, p1->y, get_stroke_ink( i, j ) ) ;
#endif
        }

    break ;   //  case RENDER_MODE_LINES:

    default:
    break ;
  }
}


void
world_finalize
( )
{
#ifndef GIF
  sampler_finalize( ) ;
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
 	accel_data_service_subscribe( 0, accel_data_service_handler ) ;
#endif

  // Trigger call to launch animation, will self repeat.
  world_update_timer_handler( NULL ) ;
}


void
world_stop
( )
{
  // Stop animation.
  if (s_world_updateTimer_ptr != NULL)
    app_timer_cancel( s_world_updateTimer_ptr ) ;

#ifndef GIF
  // Gravity unaware.
  accel_data_service_unsubscribe( ) ;
#endif
}


void
unobstructed_area_change_handler
( AnimationProgress progress
, void             *context
)
{
  available_screen = layer_get_unobstructed_bounds( s_window_layer ).size ;
}


void
click_config_provider
( void *context )
{
  window_single_click_subscribe( BUTTON_ID_UP
                               , (ClickHandler) colorMode_change_click_handler
                               ) ;

  window_single_click_subscribe( BUTTON_ID_SELECT
                               , (ClickHandler) renderMode_change_click_handler
                               ) ;

#ifdef GIF
  window_single_click_subscribe( BUTTON_ID_DOWN
                               , (ClickHandler) gifStepper_advance_click_handler
                               ) ;
#else
  #ifdef PBL_COLOR
  window_single_click_subscribe( BUTTON_ID_DOWN
                               , (ClickHandler) antialiasing_change_click_handler
                               ) ;
  #else
  window_single_click_subscribe( BUTTON_ID_DOWN
                               , (ClickHandler) invert_change_click_handler
                               ) ;
  #endif
#endif
}


void
window_load
( Window *window )
{
  s_window_layer    = window_get_root_layer( window ) ;
  available_screen  = layer_get_unobstructed_bounds( s_window_layer ).size ;

  s_action_bar_layer = action_bar_layer_create( ) ;
  action_bar_layer_add_to_window( s_action_bar_layer, s_window ) ;
  action_bar_layer_set_background_color( s_action_bar_layer, s_background_color) ;
  action_bar_layer_set_click_config_provider( s_action_bar_layer, click_config_provider ) ;

  s_world_layer = layer_create( layer_get_frame( s_window_layer ) ) ;
  layer_set_update_proc( s_world_layer, world_draw ) ;
  layer_add_child( s_window_layer, s_world_layer ) ;

  // Obstrution handling.
  UnobstructedAreaHandlers unobstructed_area_handlers = { .change = unobstructed_area_change_handler } ;
  unobstructed_area_service_subscribe( unobstructed_area_handlers, NULL ) ;

  world_start( ) ;
}


void
window_unload
( Window *window )
{
  world_stop( ) ;
  unobstructed_area_service_unsubscribe( ) ;
  layer_destroy( s_world_layer ) ;
}


void
app_init
( void )
{
  world_initialize( ) ;

  s_window = window_create( ) ;
  window_set_background_color( s_window, s_background_color ) ;
 
  window_set_window_handlers( s_window
                            , (WindowHandlers)
                              { .load   = window_load
                              , .unload = window_unload
                              }
                            ) ;

  window_stack_push( s_window, false ) ;
}


void
app_deinit
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
  app_init( ) ;
  app_event_loop( ) ;
  app_deinit( ) ;
}