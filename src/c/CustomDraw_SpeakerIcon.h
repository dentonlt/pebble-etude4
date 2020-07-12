// custom speaker icon (SVG) settings, etc.

extern GColor Custom_speaker_icon_stroke_color; // default is white 
extern GColor Custom_speaker_icon_fill_color; // clear on b/w 
extern GColor Custom_speaker_icon_silent_color; // default is white; the silent marker 
extern int  Custom_speaker_icon_stroke_width; // when 0, use calculated value

// the draw function
void Custom_speaker_icon_draw(GContext *ctx, GRect bounds);
