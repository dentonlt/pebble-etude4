// WatchFace Etude 4 
// dentonlt
//
// CHANGES
//   20200104 - first version 
//   20200106 - added aplite support, quiet time icon, debug routine.

/* calls the Pebble SDK */
#include <pebble.h>
#include "CustomDraw_BluetoothIcon.h"
#include "CustomDraw_SpeakerIcon.h"

#define null '\0'
// #define DEBUG

////////////////////////
/////////////////////
// GLOBALS
/////////////////////
////////////////////////

#ifdef DEBUG
/* debugging timer/aid. This rotates the bluetooth/quiet status, etc.*/
static AppTimer *debug_timer_handle;
static int debug_counter = 0;
static int debug_rate = 4000; // bump every 2s
static bool debug_bluetooth_status = false;
static bool debug_quiet_status = false;
#endif

/* the first Window object is created by declaring a static variable */
static Window *s_window; // the main watch window
static bool animated = true; // yes, this window has content that changes ...

static Layer *layer_main; // element for displaying the bluetooth connection...

/* customization stuff */
static GFont font_heading_time; // custom font object
static GFont font_body_text; // custom font object

// output text buffers ...
static char str_row_time[6]; // status line 1: the time heading
static char str_row_status[16]; // status line 2: battery, day, footsteps 

// features ...
#if defined(PBL_HEALTH)
static bool health_events_available = true; // updated on window load/start
static bool tapped = false; // true just after tap; briefly show current HR if available
#endif

// these are used by the draw_row_*() functions
#ifdef PBL_ROUND
// row dimensions (default for chalk)
static int row_bluetooth_h = 40; 
static int row_bluetooth_w = 180; 
static int row_bluetooth_y = 20;
static int row_time_h = 48;
static int row_time_w = 180;
static int row_time_y = 60;
static int row_status_h = 20; 
static int row_status_w = 180;
static int row_status_y = 120; 
#else
// row dimensions (default for aplite, basalt)
static int row_time_h = 48;
static int row_time_w = 144;
static int row_time_y = 12;
static int row_bluetooth_h = 40; 
static int row_bluetooth_w = 144; 
static int row_bluetooth_y = 75;
static int row_status_h = 20; 
static int row_status_w = 144;
static int row_status_y = 131; 
#endif

// //////////////////////
// persistent data storage details
// //////////////////////

// key for accessing data in the Pebble Persistent Data store/table
static const uint32_t PERSISTENTDATA_KEY = 86;

// container type for our persistent data (easy, single read/write)
struct PersistentData_Type { 
  GColor BackgroundColor; 
  bool StatusTextColorAuto;
  GColor StatusTextColor;
  bool TimeTextColorAuto;
  GColor TimeTextColor;
  };  

// persistent data storage container/object instance
static struct PersistentData_Type settings;

#ifdef DEBUG
////////////////////////
////////////////////
// DEBUG FUNCTIONS
////////////////////
////////////////////////

static void debug_timer_callback(void *data) {

  debug_counter++;

  if(debug_counter%3 == 0)
    debug_bluetooth_status = !debug_bluetooth_status;

  if(debug_counter%2 == 0) 
    debug_quiet_status = !debug_quiet_status;

  if(debug_counter >= 12)
    debug_counter = 0;
 
  debug_timer_handle = app_timer_register(debug_rate, debug_timer_callback, 0);
  layer_mark_dirty(layer_main); 
}

#endif // DEBUG

////////////////////////
////////////////////
// SUPPORT/REFERENCE FUNCTIONS
////////////////////
////////////////////////

// write our settings to memory 
void write_settings() {

  // jic ... delete existing data
  persist_delete(PERSISTENTDATA_KEY);
  // write our settings data
  persist_write_data(PERSISTENTDATA_KEY, &settings, sizeof(settings)); 
}

// read our persistent data. If there is nothing to read, fill in default values.
void read_settings() {
  if(persist_exists(PERSISTENTDATA_KEY))
  { // use existing data ...
    persist_read_data(PERSISTENTDATA_KEY, &settings, sizeof(settings));
    if(settings.TimeTextColorAuto == true)
      settings.TimeTextColor = gcolor_legible_over(settings.BackgroundColor);
    if(settings.StatusTextColorAuto == true)
      settings.StatusTextColor = gcolor_legible_over(settings.BackgroundColor);
  }
  else
  { // use default colors
    // default background is black on Square pebbles, white on Round.
    #ifdef PBL_ROUND 
    settings.TimeTextColor = GColorBlack;
    settings.StatusTextColor = GColorBlack;
    settings.BackgroundColor = GColorWhite;
    #else
    settings.TimeTextColor = GColorWhite;
    settings.StatusTextColor = GColorWhite;
    settings.BackgroundColor = GColorBlack;
    #endif
    settings.TimeTextColorAuto = true;
    settings.StatusTextColorAuto = true;
  }
}

// get day of week + day of month as [MTWRFSU][1-31]  
static char *get_day_string(struct tm *tick_time) {

  static char str_day[5];
  strftime(str_day, sizeof(str_day), "%u%d", tick_time);

  switch(str_day[0])
  {
    case '1': str_day[0] = 'M';
      break;;
    case '2': str_day[0] = 'T';
      break;;
    case '3': str_day[0] = 'W';
      break;;
    case '4': str_day[0] = 'R';
      break;;
    case '5': str_day[0] = 'F';
      break;;
    case '6': str_day[0] = 'S';
      break;;
    case '7': str_day[0] = 'U';
      break;;
    default: str_day[0] = '-'; // really shouldn't happen ...
  }

  // remove leading 0 on day of month
  if(str_day[1] == '0')
  {
    str_day[1] = str_day[2];
    str_day[2] = null; // closing null ...
  }

  return str_day;

}

#if defined(PBL_HEALTH)
/* get total steps today*/
static char *get_steps() {
  
  static char str_steps[7]; 
  str_steps[0] = null;
  HealthMetric metric = HealthMetricStepCount;
  time_t start = time_start_of_today();
  time_t end = time(NULL);

  // Check the metric has data available for today
  HealthServiceAccessibilityMask mask = health_service_metric_accessible(metric, start, end);

  if(mask & HealthServiceAccessibilityMaskAvailable) 
  { 
#ifdef DEBUG
    snprintf(str_steps, sizeof(str_steps), " 10385");
#else
    snprintf(str_steps, sizeof(str_steps), " %d", (int)health_service_sum_today(metric));
#endif
  }

  return str_steps; 
}

/* fetch current heart rate ... */
static char *get_HR() {

  static char str_HR[8];
  str_HR[0] = get_steps(); // JIC ...
  HealthMetric metric = HealthMetricHeartRateBPM; 
  HealthServiceAccessibilityMask mask = 
    health_service_metric_accessible(metric, time(NULL), time(NULL)); 
  
  if(mask & HealthServiceAccessibilityMaskAvailable)
  {
    HealthValue val = health_service_peek_current_value(metric);
    if(val > 0)
      snprintf(str_HR, sizeof(str_HR), "HR ", uint32_t(val));
  } 
  return str_HR; 
}

/* fetch either steps or HR ... determined by global bool 'tapped' */
static char *get_health_string() {

  if(tapped)
    return get_steps();
  return get_HR();

}

#endif

////////////////////////
////////////////////
// DRAW FUNCTIONS
////////////////////
////////////////////////

/* draw the time text */
static void draw_row_time(GContext *ctx, GRect bounds) {
  
  time_t temp = time(NULL); // get tm struct
  struct tm *tick_time = localtime(&temp);

  // write current time ...
  strftime(str_row_time, sizeof(str_row_time), 
	clock_is_24h_style() ? "%H:%M" : "%I:%M", 
	tick_time);

  // strip leading zero if present ... (bump all characters left by one)
  if(str_row_time[0] == '0')
    for(unsigned int i = 0; i < sizeof(str_row_time) - 1; i++)
      str_row_time[i] = str_row_time[i+1];

  // do the draw.
  graphics_context_set_text_color(ctx, settings.TimeTextColor);
  graphics_draw_text(ctx, str_row_time, font_heading_time, bounds,
	GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL); 
   
}

/* draw bluetooth icon row */ 
static void draw_row_bluetooth(GContext *ctx, GRect bounds) {

  if(PBL_IF_COLOR_ELSE(true, false))
  { // color pebbles

    // connect blue, disconnect red
#ifdef DEBUG
    if(debug_bluetooth_status)
#else
    if(connection_service_peek_pebble_app_connection())
#endif
    {
      Custom_bluetooth_icon_background_color = GColorBlue; 
      Custom_bluetooth_icon_outline_color = GColorElectricUltramarine;
      Custom_bluetooth_icon_letter_color = GColorWhite;
    }
    else
    {
      Custom_bluetooth_icon_background_color = GColorDarkCandyAppleRed;
      Custom_bluetooth_icon_outline_color = GColorBulgarianRose;
      Custom_bluetooth_icon_letter_color = GColorWhite;
    }
  }
  else // PBL_BW
  { 
      // black window, so always show black oval w/ white outline
      Custom_bluetooth_icon_outline_color = GColorWhite;
      Custom_bluetooth_icon_background_color = GColorBlack;

      // connect white letter, disconnect black letter (empty oval ...)
#ifdef DEBUG
      if(debug_bluetooth_status)
#else
      if(connection_service_peek_pebble_app_connection())
#endif
        Custom_bluetooth_icon_letter_color = GColorWhite;
      else
        Custom_bluetooth_icon_letter_color = GColorBlack; 
  }

  // check quiet time status ...
#ifdef DEBUG
  if(debug_quiet_status)
#else
  if(quiet_time_is_active()) 
#endif
  { // when quiet ... drop in the silent speaker icon
    Custom_speaker_icon_stroke_width = 3;
    Custom_speaker_icon_fill_color = 
      PBL_IF_COLOR_ELSE(GColorDarkCandyAppleRed, gcolor_legible_over(settings.BackgroundColor)); 
    Custom_speaker_icon_silent_color = 
      PBL_IF_COLOR_ELSE(GColorRed, gcolor_legible_over(settings.BackgroundColor));
    Custom_speaker_icon_stroke_color = PBL_IF_COLOR_ELSE(GColorBulgarianRose, GColorClear);
    
    // draw the speaker ...
    bounds.size.w = bounds.size.w*2/3; 
    Custom_speaker_icon_draw(ctx, bounds);
    
    // prep to push BT icon to the right
    bounds.origin.x = bounds.size.w/2;
  }

  // do the draw!
  Custom_bluetooth_icon_outline = true;
  Custom_bluetooth_icon_stroke_width = 3; 
  Custom_bluetooth_icon_draw(ctx, bounds); 
     
} // end draw_row_bluetooth() 

/* draw the status text row ... */
static void draw_row_status(GContext *ctx, GRect bounds) {
 
 // set up our strings ...
 static char str_charging[2] = { null, null };
 static char str_batt[6];
 static char str_remainder[12]; 

 // get charging state 
 BatteryChargeState state = battery_state_service_peek(); 
 if(state.is_charging)
   str_charging[0] = '+'; 
 else
   str_charging[0] = null;
 
 // get date - used below to generate date string 
 time_t temp = time(NULL); // get tm struct
 struct tm *tick_time = localtime(&temp);

 // concat string ...
 snprintf(str_row_status, sizeof(str_row_status), "%u%%%s %s%s", 
  state.charge_percent, str_charging, get_day_string(tick_time), 
#if defined(PBL_HEALTH)
  get_steps());
#else
  "");
#endif

#ifdef DEBUG
  if(debug_counter%3 == 0)
  {
    state.is_charging = true;
    str_charging[0] = '+'; 
  }
  else
  {
    state.is_charging = false;
    str_charging[0] = null; 
  }

  state.charge_percent = 100 - 10*debug_counter;
  if(state.charge_percent > 100) 
    state.charge_percent = 100;
#endif  

    GColor col_battery_text = settings.StatusTextColor;
    GSize size_batt_text, size_charging_text, size_remainder_text;

    // update text color for charge level:
    if(PBL_IF_COLOR_ELSE(true, false))
    {
      if (state.charge_percent > 80) 
        col_battery_text = GColorElectricBlue;
      else if (state.charge_percent < 30) 
        col_battery_text = GColorRed;
    }

    // status string: battery state
    snprintf(str_batt, sizeof(str_batt), "%u%%",
      state.charge_percent);   
  
    // status string: everything else 
    snprintf(str_remainder, sizeof(str_remainder), " %s%s",
	get_day_string(tick_time),
#if defined(PBL_HEALTH)
        get_steps());
#else
        "");
#endif
 
    // get size of text strings 
    size_batt_text = graphics_text_layout_get_content_size(str_batt,
      font_body_text, bounds, GTextOverflowModeWordWrap, GTextAlignmentCenter);  
    size_charging_text = graphics_text_layout_get_content_size(str_charging,
      font_body_text, bounds, GTextOverflowModeWordWrap, GTextAlignmentCenter);  
    size_remainder_text = graphics_text_layout_get_content_size(str_remainder,
      font_body_text, bounds, GTextOverflowModeWordWrap, GTextAlignmentCenter);  

    // manually center ... 
    bounds.origin.x = (bounds.size.w - size_batt_text.w 
 	- size_charging_text.w - size_remainder_text.w)/2;
 
    // battery %
    graphics_context_set_text_color(ctx, col_battery_text);
    graphics_draw_text(ctx, str_batt, font_body_text, bounds,
	GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
    bounds.origin.x += size_batt_text.w; 

    // charging status
    graphics_context_set_text_color(ctx, PBL_IF_COLOR_ELSE(GColorElectricBlue, settings.StatusTextColor));
    graphics_draw_text(ctx, str_charging, font_body_text, bounds,
	GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
    bounds.origin.x += size_charging_text.w; 

    // rest of string
    graphics_context_set_text_color(ctx, settings.StatusTextColor);
    graphics_draw_text(ctx, str_remainder, font_body_text, bounds,
      GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL); 

}

/* fill the background with a solid color (settings.BackgroundColor is global)*/
static void draw_background(GContext *ctx, GRect bounds) {
  graphics_context_set_fill_color(ctx, settings.BackgroundColor); 
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);
}

////////////////////////
////////////////////
// EVENT HANDLERS
////////////////////
////////////////////////

// listening for changes to configuration settings (message from the phone app)
static void inbox_config_handler(DictionaryIterator *di, void *context) {

  // read preferences ...
  Tuple *buf = dict_find(di, MESSAGE_KEY_BackgroundColor);
  if(buf != null) 
    settings.BackgroundColor = GColorFromHEX(buf->value->int32);

  buf = dict_find(di, MESSAGE_KEY_StatusTextColor);
  if(buf != null) 
    settings.StatusTextColor = GColorFromHEX(buf->value->int32);

  buf = dict_find(di, MESSAGE_KEY_TimeTextColor);
  if(buf != null) 
    settings.TimeTextColor = GColorFromHEX(buf->value->int32);
  
  buf = dict_find(di, MESSAGE_KEY_TimeTextColorAuto);
  if(buf != null) 
    settings.TimeTextColorAuto = (bool)buf->value->int8;
  
  buf = dict_find(di, MESSAGE_KEY_StatusTextColorAuto);
  if(buf != null) 
    settings.StatusTextColorAuto = (bool)buf->value->int8;

  write_settings(); // save settings to memory on watch
  read_settings(); // sanity-checks the values
  layer_mark_dirty(layer_main); // redraw
}

// this is the main "draw the graphics" function, but it 
// doesn't do any drawing itself. Instead, it calls the three
// draw_row_* functions above.
static void update_layer_main(Layer *layer, GContext *ctx) {

 // just pass our bounds/GContext to each row update function.
 
 static GRect bounds;
 bounds.origin.x = bounds.origin.y = 0;

 bounds.size.w = bounds.size.h = 180; // largest pebble. aplite/basalt will clip.
 draw_background(ctx, bounds);

 bounds.origin.y = row_time_y;
 bounds.size.w = row_time_w;
 bounds.size.h = row_time_h; 
 draw_row_time(ctx, bounds);

 bounds.origin.y = row_bluetooth_y;
 bounds.size.w = row_bluetooth_w;
 bounds.size.h = row_bluetooth_h; 
 draw_row_bluetooth(ctx, bounds);

 bounds.origin.y = row_status_y;
 bounds.size.w = row_status_w;
 bounds.size.h = row_status_h; 
 draw_row_status(ctx, bounds);  
}

/////////////////////////////
// CALLBACKS & HANDLERS //
/////////////////////////////

// timeout after tap: set tapped = false & redraw main layer 
static void timer_after_tap_callback(void *data) {
  tapped = false;
  layer_mark_dirty(layer_main);
}

// once a minute ... redraw.
static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  layer_mark_dirty(layer_main); 
}

// on connection change, just redraw. 
static void connection_handler(bool connected) {
  layer_mark_dirty(layer_main); 
}

// accel tap handler ...
static void accel_tap_handler(AccelAxisType axis, int32_t direction) {
  // on tap ... widdle the status line
  tapped = true;
  app_timer_register(3000, timer_after_tap_callback, NULL); // set a 3-second timeout 
  layer_mark_dirty(layer_main); // redraw ... 
}

#if defined(PBL_HEALTH)
// on steps change, redraw.
static void health_handler(HealthEventType event, void *context) {

  switch(event) {

    // update step counts
    case HealthEventSignificantUpdate:
    case HealthEventMovementUpdate: 
      layer_mark_dirty(layer_main); 
      break;

    // ignore these ...
    case HealthEventSleepUpdate: 
    case HealthEventHeartRateUpdate: 
    case HealthEventMetricAlert:
      break;
  }
}
#endif

////////////////////////
////////////////////
// CLEANUP 
////////////////////
////////////////////////
/* cleanup functions must appear ahead of init because these
   are attached to callbacks during init. see win_init().*/

/* cleanup anything we loaded. Called from win_destroy(). */
static void window_unload(Window *window) {

  // write persistent data to memory
  write_settings();

  // unsub, JIC
  accel_tap_service_unsubscribe();
  tick_timer_service_unsubscribe();
  connection_service_unsubscribe();
  
  // drop our resources
  fonts_unload_custom_font(font_heading_time);
  fonts_unload_custom_font(font_body_text);
 
  // destroy custom TextLayer element
  layer_destroy(layer_main);
}

// clean up by destroying window object
static void win_deinit() {
  window_destroy(s_window);
}

////////////////////////
////////////////////
// LOAD AND INIT
////////////////////
////////////////////////

/* load custom resources, custom settings ... */
static void load_resources() {
  font_heading_time = fonts_load_custom_font(
    resource_get_handle(RESOURCE_ID_FONT_HEADING_TIME_42));

  font_body_text = fonts_load_custom_font(
    resource_get_handle(RESOURCE_ID_FONT_BODY_TEXT_16)); 

  read_settings();
}

/* set up our graphics layer */
static void window_load(Window *window) {

  // create & configure our layer 
  Layer *win_layer = window_get_root_layer(window);
  layer_main = layer_create(layer_get_bounds(win_layer));
  layer_set_update_proc(layer_main, update_layer_main);
  
  // push the layer onto the Window/root
  layer_add_child(win_layer, layer_main);

#ifdef DEBUG
  debug_timer_handle = app_timer_register(debug_rate, debug_timer_callback, 0);
#endif

}

/* this is called by main. 
   In turn, win_init() calls our load functions, attaches handlers, etc. */
static void win_init(void) {

  /* load custom resources */
  load_resources();

  /* listen for configuration settings from phone */
  app_message_register_inbox_received(inbox_config_handler);
  app_message_open(128, 128);

  /* make main window object */
  s_window = window_create(); 

  /* attach our event handlers ... */
  accel_tap_service_subscribe(accel_tap_handler);
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  connection_service_subscribe((ConnectionHandlers) { 
  	.pebble_app_connection_handler = connection_handler
  });

  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });

#if defined(PBL_HEALTH)
  if(!health_service_events_subscribe(health_handler, NULL))
    health_events_available = false;
#endif

 /* show window on the watch */ 
 window_stack_push(s_window, animated);

 /* manually trigger update ... */
  connection_handler(connection_service_peek_pebble_app_connection());

}

////////////////////////
////////////////////
// MAIN 
////////////////////
////////////////////////

/* typical structure of main(): 
  init, event loop, deinit */
int main() {
  win_init();
  app_event_loop();
  win_deinit();
}
