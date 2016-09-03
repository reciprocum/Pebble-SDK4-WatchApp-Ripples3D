/*
   WatchApp: F3D Q3
   File    : main.c
   Author  : Afonso Santos, Portugal

   Last revision: 17h34 September 01 2016
*/

#include <pebble.h>
#include <karambola/Q3.h>
#include <karambola/CamQ3.h>
#include <karambola/Sampler.h>

#include "main.h"
#include "Config.h"

// Obstruction related.
static GSize available_screen ;


// UI related
static Window         *s_window ;
static Layer          *s_window_layer ;
static Layer          *s_world_layer ;
static ActionBarLayer *s_action_bar;


// World related
Q3 fxy[GRID_XY_LINES][GRID_XY_LINES] ;
const Q gridXY_scale = Q_from_float(GRID_XY_SCALE) ;

GPoint screenPoints[GRID_XY_LINES][GRID_XY_LINES] ;

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
world_initialize
( )
{
  sampler_initialize( ) ;

  Q halfScale         = gridXY_scale >> 1 ;   // scale / 2
  Q gridXY_numStripes = Q_from_int(GRID_XY_LINES - 1) ;
  Q gridStep          = Q_div( gridXY_scale, gridXY_numStripes ) ;
  Q x                 = -halfScale ;

  for (int i = 0  ;  i < GRID_XY_LINES  ;  ++i)
  {
    Q y = -halfScale ;

    for (int j = 0  ;  j < GRID_XY_LINES  ;  ++j)
    {
      fxy[i][j].x = x ;
      fxy[i][j].y = y ;

      y += gridStep ;
    }

    x += gridStep ;
  }

  // Initialize cam rotation constants.
  s_cam_rotZangle     = 0 ;
  s_cam_rotZstepAngle = TRIG_MAX_ANGLE / 512 ;
}


// UPDATE CAMERA & WORLD OBJECTS PROPERTIES

void
world_update
( )
{
  ++s_world_updateCount ;

  // Update z = f( x, y )
  int32_t phaseAngle = TRIG_MAX_ANGLE - ((s_world_updateCount << 8) & 0xFFFF) ;

  for (int i = 0  ;  i < GRID_XY_LINES  ;  ++i)
  {
    const Q x  = fxy[i][0].x ;
    const Q x2 = Q_mul(x, x) ;

    for (int j = 0  ;  j < GRID_XY_LINES  ;  ++j)
    {
      const Q y  = fxy[0][j].y ;
      const Q y2 = Q_mul(y, y) ;

      Q dist2 = x2 + y2 ;
      Q dist  = Q_sqrt(dist2) ;
      int32_t angle = ((dist >> 1) + phaseAngle) & 0xFFFF ;
      fxy[i][j].z = cos_lookup( angle ) ;
    }
  }


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
#ifdef QEMU
    if (ad.x == 0  &&  ad.y == 0  &&  ad.z == -1000)   // Under QEMU with SENSORS off this is the default output.
    {
      Sampler_push( sampler_accelX,  -81 ) ;
      Sampler_push( sampler_accelY, -816 ) ;
      Sampler_push( sampler_accelZ, -571 ) ;
    }
    else                                               // If running under QEMU the SENSOR feed must be ON.
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

  // this will queue a defered call to the world_draw( ) method. (% 2PI)
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
  // Disable antialiasing if running under QEMU (crashes after a few frames otherwise).
#ifdef QEMU
  graphics_context_set_antialiased( gCtx, false ) ;
#endif

  graphics_context_set_stroke_color( gCtx, GColorWhite ) ;

  // View f(x,y): calculates device coordinates of points.
  {
    const Q  k = ( available_screen.w > available_screen.h ) ? Q_from_int(available_screen.h)
                                                             : Q_from_int(available_screen.w)
                                                             ;
    const Q  bX = Q_from_int(available_screen.w) >> 1 ;
    const Q  bY = Q_from_int(available_screen.h) >> 1 ;

    for (int i = 0  ;  i < GRID_XY_LINES  ;  ++i)
      for (int j = 0  ;  j < GRID_XY_LINES  ;  ++j)
      {
        // calculate camera coordinates of visible vertex.
        Q2 vCamera ;
        CamQ3_view( &vCamera, &s_cam, &fxy[i][j] ) ;
  
        // calculate device coordinates of vertex.
        Q x = Q_mul( k, vCamera.x )  +  bX ;
        Q y = Q_mul( k, vCamera.y )  +  bY ;
        screenPoints[i][j].x = Q_to_int(x) ;
        screenPoints[i][j].y = Q_to_int(y) ;

        graphics_draw_pixel( gCtx, screenPoints[i][j] ) ;
      }
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

  // Call me again.
  s_world_updateTimer_ptr = app_timer_register( ANIMATION_INTERVAL_MS, world_update_timer_handler, data ) ;
}


void
world_start
( )
{
  // Set initial world mode (and subscribe to related services).
 	accel_data_service_subscribe( 0, accel_data_service_handler ) ;

  // Trigger call to launch animation, will self repeat.
  world_update_timer_handler( NULL ) ;
}


void
world_stop
( )
{
  // Stop animation.
  app_timer_cancel( s_world_updateTimer_ptr ) ;

  // Gravity unaware.
  accel_data_service_unsubscribe( ) ;
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
window_load
( Window *s_window )
{
  s_window_layer    = window_get_root_layer( s_window ) ;
  available_screen  = layer_get_unobstructed_bounds( s_window_layer ).size ;

  s_action_bar = action_bar_layer_create( ) ;
  action_bar_layer_add_to_window( s_action_bar, s_window ) ;

  GRect bounds = layer_get_frame( s_window_layer ) ;
  s_world_layer = layer_create( bounds ) ;
  layer_set_update_proc( s_world_layer, world_draw ) ;
  layer_add_child( s_window_layer, s_world_layer ) ;

  // Obstrution handling.
  UnobstructedAreaHandlers unobstructed_area_handlers = { .change = unobstructed_area_change_handler } ;
  unobstructed_area_service_subscribe( unobstructed_area_handlers, NULL ) ;

  // Position s_cube handles according to current time, launch blinkers, launch animation, start s_cube.
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
