/*
   WatchApp: Ripples 3D
   File    : main.c
   Author  : Afonso Santos, Portugal

   Last revision: 13h34 September 03 2016  GMT
*/

#include <pebble.h>
#include <karambola/Q3.h>
#include <karambola/CamQ3.h>
#include <karambola/Sampler.h>
//#include <karambola/Draw2D.h>

#include "main.h"
#include "Config.h"

// Obstruction related.
static GSize available_screen ;


// UI related
static Window         *s_window ;
static Layer          *s_window_layer ;
static Layer          *s_world_layer ;
static ActionBarLayer *s_action_bar;


static RenderMode    s_render_mode  = RENDER_MODE_DEFAULT ;


// World related
Q3     worldPoints [GRID_LINES][GRID_LINES] ;
Q      dist_xy     [GRID_LINES][GRID_LINES] ;
GPoint screenPoints[GRID_LINES][GRID_LINES] ;

const Q  grid_scale = Q_from_float(GRID_SCALE) ;

static int        s_world_updateCount       = 0 ;
static AppTimer  *s_world_updateTimer_ptr   = NULL ;

Sampler   *sampler_accelX = NULL ;            // To be allocated at world_initialize( ).
Sampler   *sampler_accelY = NULL ;            // To be allocated at world_initialize( ).
Sampler   *sampler_accelZ = NULL ;            // To be allocated at world_initialize( ).


// Camera related

static CamQ3   s_cam ;
static Q       s_cam_zoom        = PBL_IF_RECT_ELSE(Q_from_float(1.25f), Q_from_float(1.15f)) ;
static int32_t s_cam_rotZangle   = 0 ;
static int32_t s_cam_rotZstepAngle ;


// Acellerometer handlers.
void
accel_data_service_handler
( AccelData *data
, uint32_t   num_samples
)
{ }


// Forward declare.
void  world_stop( ) ;
void  world_finalize( ) ;


void
cam_config
( const Q3      *pViewPoint
, const int32_t  pRotZangle
)
{
  Q3 scaledVP ;
  Q3_scaTo( &scaledVP, CAM3D_DISTANCEFROMORIGIN, pViewPoint ) ;

  Q3 rotatedVP ;
  Q3_rotZ( &rotatedVP, &scaledVP, pRotZangle ) ;

  // setup 3D camera
  CamQ3_lookAtOriginUpwards( &s_cam, &rotatedVP, s_cam_zoom, CAM_PROJECTION_PERSPECTIVE ) ;
}


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


void
world_initialize
( )
{
  sampler_initialize( ) ;
  function_initialize( ) ;

  // Initialize cam rotation vars.
  s_cam_rotZangle     = 0 ;
  s_cam_rotZstepAngle = TRIG_MAX_ANGLE >> 9 ;  //  2 * PI / 512
}


// UPDATE CAMERA & WORLD OBJECTS PROPERTIES

void
function_update
( )
{
  // Update z = f( x, y )
  int32_t anglePhase = TRIG_MAX_ANGLE - ((s_world_updateCount << 8) & 0xFFFF) ;

  for (int i = 0  ;  i < GRID_LINES  ;  ++i)
    for (int j = 0  ;  j < GRID_LINES  ;  ++j)
    {
      int32_t angle = ((dist_xy[i][j] >> 1) + anglePhase) & 0xFFFF ;
      worldPoints[i][j].z = cos_lookup( angle ) ;
    }
}


void
camera_update
( )
{
#ifndef GIF
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
#endif

  const float kAvg = 0.001f / sampler_accelX->samplesNum ;
  const float avgX = (float)(kAvg * sampler_accelX->samplesAcum ) ;
  const float avgY =-(float)(kAvg * sampler_accelY->samplesAcum ) ;
  const float avgZ =-(float)(kAvg * sampler_accelZ->samplesAcum ) ;

  static Q3 viewPoint ;
  viewPoint.x = Q_from_float( avgX ) ;
  viewPoint.y = Q_from_float( avgY ) ;
  viewPoint.z = Q_from_float( avgZ ) ;
      
  s_cam_rotZangle += s_cam_rotZstepAngle ;
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


#ifdef LOG
static int s_world_draw_count = 0 ;
#endif

void
world_draw
( Layer    *me
, GContext *gCtx
)
{
#ifdef RAW
  // Disable antialiasing if running under EMU (crashes after a few frames otherwise).
  graphics_context_set_antialiased( gCtx, false ) ;
#endif

#ifndef PBL_COLOR
  graphics_context_set_stroke_color( gCtx, GColorWhite ) ;
#endif

  // View f(x,y)
  const Q  k = (available_screen.w > available_screen.h)
             ? Q_from_int(available_screen.h)
             : Q_from_int(available_screen.w)
             ;

  const Q  bX = Q_from_int(available_screen.w) >> 1 ;
  const Q  bY = Q_from_int(available_screen.h) >> 1 ;

  // Calculate screen points.
  for (int i = 0  ;  i < GRID_LINES  ;  ++i)
    for (int j = 0  ;  j < GRID_LINES  ;  ++j)
    {
      // calculate camera coordinates of visible vertex.
      Q2 vCamera ;
      CamQ3_view( &vCamera, &s_cam, &worldPoints[i][j] ) ;
  
      // calculate device coordinates of vertex.
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
          graphics_context_set_stroke_color( gCtx, GColorWhite ) ;
#endif

          graphics_draw_pixel( gCtx, screenPoints[i][j] ) ;
        }
    break ;

    case RENDER_MODE_LINES:
      // y parallel lines.
      for (int i = 0  ;  i < GRID_LINES  ;  ++i)
        for (int j = 1  ;  j < GRID_LINES  ;  ++j)
        {
#ifdef PBL_COLOR
          graphics_context_set_stroke_color( gCtx, GColorYellow ) ;
#endif

          GPoint *p1 = &screenPoints[i][j] ;
          GPoint *p2 = &screenPoints[i][j-1] ;
          graphics_draw_line( gCtx, *p1, *p2 ) ;
        }

      // x parallel lines.
      for (int j = 0  ;  j < GRID_LINES  ;  ++j)
        for (int i = 1  ;  i < GRID_LINES  ;  ++i)
        {
#ifdef PBL_COLOR
          graphics_context_set_stroke_color( gCtx, GColorGreen ) ;
#endif

          GPoint *p1 = &screenPoints[i][j] ;
          GPoint *p2 = &screenPoints[i-1][j] ;
          graphics_draw_line( gCtx, *p1, *p2 ) ;
        }

    break ;

    default:
    break ;
  }
}


static
void
sampler_finalize
( )
{
  free( Sampler_free( sampler_accelX ) ) ; sampler_accelX = NULL ;
  free( Sampler_free( sampler_accelY ) ) ; sampler_accelY = NULL ;
  free( Sampler_free( sampler_accelZ ) ) ; sampler_accelZ = NULL ;
}


void
world_finalize
( )
{
  sampler_finalize( ) ;
}


void
world_update_timer_handler
( void *data )
{
  world_update( ) ;

#ifndef GIF
  // Call me again.
  s_world_updateTimer_ptr = app_timer_register( ANIMATION_INTERVAL_MS, world_update_timer_handler, data ) ;
#endif
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


void
click_config_provider
( void *context )
{
  window_single_click_subscribe( BUTTON_ID_SELECT
                               , (ClickHandler) renderMode_change_click_handler
                               ) ;
}


void
window_load
( Window *s_window )
{
  s_window_layer    = window_get_root_layer( s_window ) ;
  available_screen  = layer_get_unobstructed_bounds( s_window_layer ).size ;

  s_action_bar = action_bar_layer_create( ) ;
  action_bar_layer_add_to_window( s_action_bar, s_window ) ;
  action_bar_layer_set_click_config_provider( s_action_bar, click_config_provider ) ;

  GRect bounds = layer_get_frame( s_window_layer ) ;
  s_world_layer = layer_create( bounds ) ;
  layer_set_update_proc( s_world_layer, world_draw ) ;
  layer_add_child( s_window_layer, s_world_layer ) ;

  // Obstrution handling.
  UnobstructedAreaHandlers unobstructed_area_handlers = { .change = unobstructed_area_change_handler } ;
  unobstructed_area_service_subscribe( unobstructed_area_handlers, NULL ) ;

  world_start( ) ;
}


void
window_unload
( Window *s_window )
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
  window_set_background_color( s_window, GColorBlack ) ;
 
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