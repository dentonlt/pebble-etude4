#include <pebble.h>
#include <math.h>

// #define DEBUG

// externally accessible - settings
int    Custom_bluetooth_icon_stroke_width = 0; // only used if set >0 by user
bool   Custom_bluetooth_icon_background = true;
bool   Custom_bluetooth_icon_outline = false; 

GColor Custom_bluetooth_icon_letter_color = { (uint8_t)0b11111111 }; // argb

GColor Custom_bluetooth_icon_background_color = { 
  (uint8_t)PBL_IF_COLOR_ELSE(0b11010011, 0b11111111) }; // argb

GColor Custom_bluetooth_icon_outline_color = { (uint8_t)0b11111111 }; // argb

// our source dimensions, etc.
  // we only need partial coordinates and the two dimensions 
  // X coords x3
  // Y coords x4
  // desired image width (from layer)
  // desired image height (from layer)

// source data ... 
typedef struct {
  int X;
  int Y;
  int H;
  int W;
  int Xa;
  int Xb;
  int Xc;
  int Ya;
  int Yb;
  int Yc;
  int Yd;
  int stroke_width;
} ImageScalars_BluetoothIcon;

// default values: these come from the original SVG file/image
static const ImageScalars_BluetoothIcon SrcScalars = { 0, 0, 50, 32, 8, 16, 23, 9, 17, 32, 41, 3 };
static ImageScalars_BluetoothIcon TargetScalars = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

// Path storage. This needs to exist until we update the screen.
// these will be updated using the values from TargetScalars
static GPath *path;
static GPathInfo path_info; 

static void fill_path_data() {
  // use the information in TargetScalars to fill the GPathInfo struct
  
  path_info.num_points = 6; 
  path_info.points = (GPoint []) {
      { TargetScalars.Xa, TargetScalars.Yb },
      { TargetScalars.Xc, TargetScalars.Yc },
      { TargetScalars.Xb, TargetScalars.Yd },
      { TargetScalars.Xb, TargetScalars.Ya },
      { TargetScalars.Xc, TargetScalars.Yb },
      { TargetScalars.Xa, TargetScalars.Yc }
    };

  // create the path. This must be destroy'd after use.
  path = gpath_create(&path_info);
}

// within source_bounds, find largest icon we can draw ...
static void fill_bluetooth_target_scalars(GRect source_bounds) {

  // we know the slope of our icon is based on the ratio 32w : 50h.
  // We just descend down that slope until we hit one of the source_bounds.
  // Use a variant on Bresenham to work out the point of intersection.
  // That point defines the size of our largest possible image. We use that.
  
  int dx = SrcScalars.W, sx = 1;
  int dy = SrcScalars.H, sy = 1; 
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
  TargetScalars.W = x0;
  TargetScalars.H = y0;

  // now get the scale for our points ...
  float ratio = (float)TargetScalars.W / (float)SrcScalars.W;

  // and the translation to center the image within the requested bounds.
  int xdif = (source_bounds.size.w - TargetScalars.W) / 2 + source_bounds.origin.x;
  int ydif = (source_bounds.size.h - TargetScalars.H) / 2 + source_bounds.origin.y;

  TargetScalars.X = xdif;
  TargetScalars.Xa = (int)round((float)SrcScalars.Xa * ratio) + xdif;
  TargetScalars.Xb = (int)round((float)SrcScalars.Xb * ratio) + xdif;
  TargetScalars.Xc = (int)round((float)SrcScalars.Xc * ratio) + xdif;

  TargetScalars.Y = ydif;
  TargetScalars.Ya = (int)round((float)SrcScalars.Ya * ratio) + ydif;
  TargetScalars.Yb = (int)round((float)SrcScalars.Yb * ratio) + ydif; 
  TargetScalars.Yc = (int)round((float)SrcScalars.Yc * ratio) + ydif;
  TargetScalars.Yd = (int)round((float)SrcScalars.Yd * ratio) + ydif;

  TargetScalars.stroke_width = (int)round((float)SrcScalars.stroke_width * ratio) - 1;
    if(TargetScalars.stroke_width < 1)
      TargetScalars.stroke_width = 1;
    else if(TargetScalars.stroke_width%2 != 0) // has to be an odd number ...
      TargetScalars.stroke_width -= 1;

  // send it all to the path struct!
  fill_path_data();

}

/* this will draw the bluetooth icon onto ctx, expanded to fill bounds.
   If invert_colors is true, then draw w/ swapped letter/oval colors. */
void Custom_bluetooth_icon_draw(GContext *ctx, GRect bounds) {
 
  // scale the icon to fit current layer 
  fill_bluetooth_target_scalars(bounds);  

  // shortcuts to long variable names
  #define color_fg Custom_bluetooth_icon_letter_color
  #define color_bg Custom_bluetooth_icon_background_color
  #define color_bgo Custom_bluetooth_icon_outline_color

#ifdef DEBUG 
 APP_LOG(APP_LOG_LEVEL_DEBUG, "draw() colors letter/bg/outline: %x, %x, %x", 
   color_fg.argb, color_bg.argb, color_bgo.argb);
#endif

  GRect rect = GRect(TargetScalars.X, TargetScalars.Y, TargetScalars.W, TargetScalars.H); 

  // draw the oval background 
  if(Custom_bluetooth_icon_background) 
  {
    graphics_context_set_fill_color(ctx, color_bg);
    graphics_fill_rect(ctx, rect, TargetScalars.W/2, GCornersAll);
  }

  // draw outline on edge of oval 
  if(Custom_bluetooth_icon_outline)
  {
    graphics_context_set_stroke_width(ctx, 1); // default ...
    graphics_context_set_stroke_color(ctx, color_bgo);
    graphics_draw_round_rect(ctx, rect, TargetScalars.W/2);   
  }
 
  // draw letter 
  graphics_context_set_stroke_color(ctx, color_fg);
  if(Custom_bluetooth_icon_stroke_width != 0)
    graphics_context_set_stroke_width(ctx, Custom_bluetooth_icon_stroke_width);
  else
    graphics_context_set_stroke_width(ctx, TargetScalars.stroke_width);

  gpath_draw_outline_open(ctx, path);

  // cleanup ...
  gpath_destroy(path);

  // done! */

}
