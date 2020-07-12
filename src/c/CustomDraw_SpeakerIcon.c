// CustomDraw_Speaker.c
//
// dentonlt, 2020

#include <pebble.h>
#include <math.h>

// #define DEBUG

// externally accessible - settings
int    Custom_speaker_icon_stroke_width = 0; // only used if set >0 by user
GColor Custom_speaker_icon_stroke_color = { .argb = GColorWhiteARGB8 };
GColor Custom_speaker_icon_silent_color = { .argb = GColorRedARGB8 };
GColor Custom_speaker_icon_fill_color = { .argb =  
  (uint8_t)PBL_IF_COLOR_ELSE(GColorLightGrayARGB8, GColorWhiteARGB8) }; 

// Path storage. This needs to exist until we update the screen, so they're 
// these will be updated using the values from TargetScalars
static GPath *path;
static int src_icon_h = 50;
static int src_icon_w = 50;
static int src_stroke_width = 3;
static GPathInfo src_path_info = {
    .num_points = 6,
    .points = (GPoint []) {
      { 3, 16 },
      { 10, 16  },
      { 29, 3 },
      { 29, 47 },
      { 10, 34 },
      { 3, 34 }
    }
  }; 
static GPoint src_silent_cross_points[4] = { 
    { 34, 19 }, { 46, 31 },
    { 34, 31 }, { 46, 19 } 
  };	 

// these are filled by fill_target_path_info() 
static int target_icon_h, target_icon_w;
static int target_stroke_width = 3; 
static GPathInfo target_path_info = {
    .num_points = 6,
    .points = (GPoint []) {
      { 3, 16  },
      { 10, 16 },
      { 31, 3  },
      { 31, 47 },
      { 10, 34 },
      { 3, 34  }
    }
  }; 

// this probably needs to go into a GPathInfo somehow ...
static GPoint target_silent_cross_points[4] = { 
    { 33, 18 }, { 47, 32 },
    { 33, 32 }, { 47, 18 } 
  };	 


// within source_bounds, find largest icon we can draw ...
static void fill_target_path_info(GRect source_bounds) {

  // we know the slope of our icon is based on the src_path_info height. 
  // We just descend down that slope until we hit one of the source_bounds.
  // Use a variant on Bresenham to work out the point of intersection.
  // That point defines the size of our largest possible image. We use that.
  
  int dx = src_icon_w, sx = 1;
  int dy = src_icon_h, sy = 1; 
  int err = (dx>dy ? dx : -dy)/2, e2;
  int x0 = 0, x1 = source_bounds.size.w;
  int y0 = 0, y1 = source_bounds.size.h;

  // first, we get the biggest W/H we can fit in this layer ... 
  while((x0 < x1)&&(y0 < y1)) 
  {
    e2 = err;
    if (e2 >-dx) { err -= dy; x0 += sx; }
    if (e2 < dy) { err += dx; y0 += sy; }
  }
  target_icon_w = x0;
  target_icon_h = y0;

  // now get the scale for our points ...
  float ratio = (float)target_icon_w / (float)src_icon_w;

  // and the translation to center the image within the requested bounds.
  int xdif = (source_bounds.size.w - target_icon_w) / 2 + source_bounds.origin.x;
  int ydif = (source_bounds.size.h - target_icon_h) / 2 + source_bounds.origin.y;

  // apply the xdif/ydif to each point in src_path_info. output to target_path_info 

  for(unsigned i = 0; i < target_path_info.num_points; i++)
  {
    target_path_info.points[i].x =
      (int)round((float)src_path_info.points[i].x * ratio) + xdif;
   
    target_path_info.points[i].y =
      (int)round((float)src_path_info.points[i].y * ratio) + ydif;

    if(i < 4)
    { // adjust the points in the 'silent' cross
      target_silent_cross_points[i].x =
        (int)round((float)src_silent_cross_points[i].x * ratio) + xdif;
   
      target_silent_cross_points[i].y =
        (int)round((float)src_silent_cross_points[i].y * ratio) + ydif;
    }

  }

  target_stroke_width = (int)round((float)src_stroke_width * ratio) - 1;
    if(target_stroke_width < 1)
      target_stroke_width = 1;
    else if(target_stroke_width%2 != 0) // has to be an odd number ...
      target_stroke_width -= 1;

  // create the path. This must be destroy'd after use.
  path = gpath_create(&target_path_info);

}

/* this will draw the bluetooth icon onto ctx, expanded to fill bounds.
   If invert_colors is true, then draw w/ swapped letter/oval colors. */
void Custom_speaker_icon_draw(GContext *ctx, GRect bounds) {
 
  // scale the path to fit current layer 
  fill_target_path_info(bounds);  

  // shortcuts to long variable names
  #define color_fg Custom_speaker_icon_stroke_color
  #define color_fill Custom_speaker_icon_fill_color
  #define color_silent Custom_speaker_icon_silent_color

#ifdef DEBUG 
 APP_LOG(APP_LOG_LEVEL_DEBUG, "draw() colors fg/fill/silent: %x, %x, %x", 
   color_fg.argb, color_fill.argb, color_silent.argb);
#endif


  // draw body (fill color)
  if(!gcolor_equal(GColorClear, color_fill))
  {
    graphics_context_set_fill_color(ctx, color_fill); 
    gpath_draw_filled(ctx, path);
  }

  // draw outline 
  if(!gcolor_equal(GColorClear, color_fg)) 
  {
    graphics_context_set_stroke_width(ctx, 1); // default... 
    graphics_context_set_stroke_color(ctx, color_fg);
    gpath_draw_outline(ctx, path);
  }

  if(!gcolor_equal(color_silent, GColorClear))
  {
    if(Custom_speaker_icon_stroke_width != 0)
      graphics_context_set_stroke_width(ctx, Custom_speaker_icon_stroke_width);
    else
      graphics_context_set_stroke_width(ctx, target_stroke_width);

    // now we draw a little X in front of the speaker ...
    graphics_context_set_stroke_color(ctx, color_silent);
    graphics_draw_line(ctx, target_silent_cross_points[0], target_silent_cross_points[1]); 
    graphics_draw_line(ctx, target_silent_cross_points[2], target_silent_cross_points[3]); 
  }

  // cleanup ...
  gpath_destroy(path);

  // done! */
}
