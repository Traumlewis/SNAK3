#ifndef TEST_PEBBLE_H
#define TEST_PEBBLE_H
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
typedef struct { int16_t x,y; } GPoint;
#define GPoint(x,y) ((GPoint){x,y})
typedef struct { int16_t w,h; } GSize;
typedef struct { GPoint origin; GSize size; } GRect;
#define GRect(x,y,w,h) ((GRect){{x,y},{w,h}})
typedef struct Layer { GRect bounds; } Layer;
typedef struct Window { Layer root; } Window;
typedef struct AppTimer { int value; } AppTimer;
typedef void GContext;
typedef void *ClickRecognizerRef;
typedef struct { void (*load)(Window *); void (*unload)(Window *); } WindowHandlers;
enum { GColorBlack, GColorWhite, GColorRed, GColorGreen, GCornerNone, GTextOverflowModeTrailingEllipsis, GTextOverflowModeWordWrap, GTextAlignmentCenter, GTextAlignmentLeft, GTextAlignmentRight, BUTTON_ID_SELECT, BUTTON_ID_UP, BUTTON_ID_DOWN };
typedef int TimeUnits;
enum { MINUTE_UNIT = 2 };
static int test_tick_subscribes, test_tick_unsubscribes, test_tick_units;
static const char *test_clock_value = "13:47";
static int test_dirty_count;
static Layer *test_dirty_layer;
typedef struct { char text[64]; GRect rect; int align; } TestText;
static TestText test_text[16];
static int test_text_count, test_line_count;
static GPoint test_line_start, test_line_end;
static bool test_persist_has[8];
static int test_persist_values[8], test_persist_writes, test_persist_last_key;
static int test_light_calls, test_focus_subscribes, test_focus_unsubscribes;
static bool test_light_on;
static int test_draw_circles, test_fill_circles;
#define FONT_KEY_GOTHIC_14 "14"
#define FONT_KEY_GOTHIC_18_BOLD "18"
#define FONT_KEY_GOTHIC_28_BOLD "28"
#define FONT_KEY_GOTHIC_24_BOLD "24"
#ifdef PBL_BW
#define PBL_IF_COLOR_ELSE(a,b) (b)
#else
#define PBL_IF_COLOR_ELSE(a,b) (a)
#endif
#ifdef PBL_ROUND
#define PBL_IF_ROUND_ELSE(a,b) (a)
#else
#define PBL_IF_ROUND_ELSE(a,b) (b)
#endif
extern int test_timer_cancels, test_layer_destroys;
static inline bool persist_exists(int k) { return k>=0 && k<8 && test_persist_has[k]; }
static inline int persist_read_int(int k) { return k>=0 && k<8 ? test_persist_values[k] : 0; }
static inline int persist_write_int(int k,int v) {
  test_persist_writes++; test_persist_last_key=k;
  if(k>=0 && k<8) { test_persist_has[k]=true; test_persist_values[k]=v; }
  return sizeof(int);
}
static inline void light_enable(bool on) { test_light_calls++; test_light_on=on; }
static inline void app_focus_service_subscribe(void (*handler)(bool)) { test_focus_subscribes++; }
static inline void app_focus_service_unsubscribe(void) { test_focus_unsubscribes++; }
static inline void layer_mark_dirty(Layer *l) { test_dirty_count++; test_dirty_layer=l; }
static inline void clock_copy_time_string(char *buffer,size_t size) { snprintf(buffer,size,"%s",test_clock_value); }
static inline void tick_timer_service_subscribe(TimeUnits units,void (*cb)(struct tm *,TimeUnits)) { test_tick_subscribes++; test_tick_units=units; }
static inline void tick_timer_service_unsubscribe(void) { test_tick_unsubscribes++; }
static inline void vibes_short_pulse(void) {}
static inline AppTimer *app_timer_register(int ms,void (*cb)(void *),void *data) { static AppTimer t; return &t; }
static inline void app_timer_cancel(AppTimer *t) { test_timer_cancels++; }
static inline GRect layer_get_bounds(Layer *l) { return l->bounds; }
static inline Layer *layer_create(GRect r) { Layer *l=malloc(sizeof(*l)); l->bounds=r; return l; }
static inline void layer_destroy(Layer *l) { if(l) { test_layer_destroys++; free(l); } }
static inline void layer_set_update_proc(Layer *l,void (*fn)(Layer *,GContext *)) {}
static inline void layer_add_child(Layer *p,Layer *c) {}
static inline Layer *window_get_root_layer(Window *w) { return &w->root; }
static inline Window *window_create(void) { Window *w=calloc(1,sizeof(*w)); w->root.bounds=GRect(0,0,200,228); return w; }
static inline void window_destroy(Window *w) { free(w); }
static inline void window_set_background_color(Window *w,int color) {}
static inline void window_set_window_handlers(Window *w,WindowHandlers h) {}
static inline void window_set_click_config_provider(Window *w,void (*fn)(void *)) {}
static inline void window_stack_push(Window *w,bool a) {}
static inline void app_event_loop(void) {}
static inline void window_single_click_subscribe(int b,void (*fn)(ClickRecognizerRef,void *)) {}
static inline void window_raw_click_subscribe(int b,void (*down)(ClickRecognizerRef,void *),void (*up)(ClickRecognizerRef,void *),void *ctx) {}
static inline void window_long_click_subscribe(int b,int ms,void (*fn)(ClickRecognizerRef,void *),void (*release)(ClickRecognizerRef,void *)) {}
static inline void graphics_context_set_fill_color(GContext *c,int color) {}
static inline void graphics_context_set_text_color(GContext *c,int color) {}
static inline void graphics_context_set_stroke_color(GContext *c,int color) {}
static inline void graphics_draw_rect(GContext *c,GRect r) {}
static inline void graphics_draw_circle(GContext *c,GPoint p,int radius) { test_draw_circles++; }
static inline void graphics_fill_circle(GContext *c,GPoint p,int radius) { test_fill_circles++; }
static inline void graphics_draw_line(GContext *c,GPoint from,GPoint to) { test_line_count++; test_line_start=from; test_line_end=to; }
static inline void graphics_fill_rect(GContext *c,GRect r,int radius,int corners) {}
static inline void *fonts_get_system_font(const char *key) { return NULL; }
static inline void graphics_draw_text(GContext *c,const char *text,void *font,GRect r,int overflow,int align,void *attr) {
  if(test_text_count<16) {
    TestText *t=&test_text[test_text_count++];
    snprintf(t->text,sizeof(t->text),"%s",text); t->rect=r; t->align=align;
  }
}
#endif
