// custom bluetooth icon (SVG) settings, etc.

// The color of the background oval. If a color screen, ElectricUltramarine. Else White.
extern GColor Custom_bluetooth_icon_background_color; 
extern GColor Custom_bluetooth_icon_letter_color; // default is White 
extern GColor Custom_bluetooth_icon_outline_color; // default is White 

extern int  Custom_bluetooth_icon_stroke_width; // when 0, use calculated value

// when the icon oval is the same color as the layer/window background, an outline
// can help define the icon. Default is false.
extern bool Custom_bluetooth_icon_outline;
extern bool Custom_bluetooth_icon_background; // when true, draw background oval (default) 

// the draw function
void Custom_bluetooth_icon_draw(GContext *ctx, GRect bounds);
