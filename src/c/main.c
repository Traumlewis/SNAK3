#include <pebble.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct { int x; int y; } Point;
typedef enum { DIR_UP = 0, DIR_RIGHT, DIR_DOWN, DIR_LEFT } Direction;
static Window *s_window;
static Layer *s_game_layer;
static Layer *s_ui_layer;
static AppTimer *s_timer;
static Point s_snake[200];
static int s_snake_len;
static Point s_apple;
static Direction s_dir;
static bool s_turn_queued;
static bool s_select_long_handled;
static bool s_select_pressed;
static int s_score;
static int s_highscore;
static bool s_running;
static bool s_paused;
static bool s_game_over;
static int s_cell_size;
static int s_grid_w;
static int s_grid_h;
static int s_board_width;
static int s_board_height;
static int s_speed = 180;
static bool s_blink = false;
static bool s_light_keep_on;
static bool s_light_override;
static bool s_has_focus = true;
#define SCORE_HEIGHT 20
#define PLAYFIELD_TOP (SCORE_HEIGHT + 2)
#define BOARD_INSET PBL_IF_ROUND_ELSE(1, 0)
#define MAX_LEN 200
#define LIGHT_SETTING_KEY 2

static bool light_control_visible(void) {
  return !s_game_over && (!s_running || s_paused);
}

static void apply_light_mode(void) {
  bool force_on = s_light_keep_on && s_running && !s_paused && !s_game_over && s_has_focus;
  if(force_on != s_light_override) {
    light_enable(force_on);
    s_light_override = force_on;
  }
}

static void load_light_setting(void) {
  s_light_keep_on = persist_exists(LIGHT_SETTING_KEY) && persist_read_int(LIGHT_SETTING_KEY) == 1;
}

static void focus_handler(bool in_focus) {
  s_has_focus = in_focus;
  apply_light_mode();
}

static void configure_board(GSize size) {
  s_cell_size = size.w / 24;
  if(s_cell_size < 3) s_cell_size = 3;
  s_board_width = size.w - 2 * BOARD_INSET;
  s_board_height = size.h - PLAYFIELD_TOP - BOARD_INSET;
  s_grid_w = s_board_width / s_cell_size;
  s_grid_h = s_board_height / s_cell_size;
}

static GRect cell_rect(int x, int y) {
  // Distribute remainder pixels across the grid: no unused strip at any edge.
  int left = x * s_board_width / s_grid_w;
  int right = (x + 1) * s_board_width / s_grid_w;
  int top = y * s_board_height / s_grid_h;
  int bottom = (y + 1) * s_board_height / s_grid_h;
  return GRect(BOARD_INSET + left, PLAYFIELD_TOP + top, right - left, bottom - top);
}
static void load_highscore(void) {
  if(persist_exists(1)) { s_highscore = persist_read_int(1); }
  else { s_highscore = 0; }
}
static void save_highscore(void) {
  if(s_score > s_highscore) { s_highscore = s_score; persist_write_int(1, s_highscore); }
}
static void place_apple(void) {
  bool valid = false;
  while(!valid) {
    valid = true;
    s_apple.x = rand() % s_grid_w;
    s_apple.y = rand() % s_grid_h;
    for(int i = 0; i < s_snake_len; i++) {
      if(s_snake[i].x == s_apple.x && s_snake[i].y == s_apple.y) { valid = false; break; }
    }
  }
}
static void reset_game(void) {
  s_snake_len = 4;
  int cx = s_grid_w / 2;
  int cy = s_grid_h / 2;
  s_snake[0] = (Point){cx, cy};
  s_snake[1] = (Point){cx - 1, cy};
  s_snake[2] = (Point){cx - 2, cy};
  s_snake[3] = (Point){cx - 3, cy};
  s_dir = DIR_RIGHT;
  s_turn_queued = false;
  s_score = 0;
  s_speed = 180;
  s_running = false;
  s_paused = false;
  s_game_over = false;
  apply_light_mode();
  place_apple();
  layer_mark_dirty(s_game_layer);
  layer_mark_dirty(s_ui_layer);
}
static bool self_collision(int x, int y, bool growing) {
  // The tail moves away unless this step grows the snake.
  int occupied_len = s_snake_len - (growing ? 0 : 1);
  for(int i = 1; i < occupied_len; i++) {
    if(s_snake[i].x == x && s_snake[i].y == y) { return true; }
  }
  return false;
}
static void game_over(void) {
  s_game_over = true;
  s_running = false;
  apply_light_mode();
  save_highscore();
  vibes_short_pulse();
}
static void move_snake(void) {
  s_turn_queued = false;
  int nx = s_snake[0].x;
  int ny = s_snake[0].y;
  switch(s_dir) {
    case DIR_UP: ny--; break;
    case DIR_RIGHT: nx++; break;
    case DIR_DOWN: ny++; break;
    case DIR_LEFT: nx--; break;
  }
  if(nx < 0 || nx >= s_grid_w || ny < 0 || ny >= s_grid_h) { game_over(); return; }
  bool eating = nx == s_apple.x && ny == s_apple.y;
  bool growing = eating && s_snake_len < MAX_LEN;
  if(self_collision(nx, ny, growing)) { game_over(); return; }
  if(eating) {
    s_score++;
    if(s_score % 3 == 0 && s_speed > 80) { s_speed -= 10; }
    if(growing) { s_snake_len++; }
  }
  for(int i = s_snake_len - 1; i > 0; i--) { s_snake[i] = s_snake[i - 1]; }
  s_snake[0].x = nx;
  s_snake[0].y = ny;
  if(eating) { place_apple(); }
  layer_mark_dirty(s_game_layer);
}
static void timer_callback(void *data) {
  s_timer = NULL;
  if(s_running && !s_paused && !s_game_over) { move_snake(); }
  s_blink = !s_blink;
  layer_mark_dirty(s_ui_layer);
  s_timer = app_timer_register(s_speed, timer_callback, NULL);
}
static void draw_header(GContext *ctx, GRect bounds) {
  graphics_context_set_text_color(ctx, GColorWhite);
  char score[24], high[24], clock_text[16];
  snprintf(score, sizeof(score), bounds.size.w < 140 ? "Score%d" : "Score %d", s_score);
  snprintf(high, sizeof(high), bounds.size.w < 140 ? "High%d" : "High %d", s_highscore);
  clock_copy_time_string(clock_text, sizeof(clock_text));
  int padding = bounds.size.w >= 160 ? 6 : (bounds.size.w < 140 ? 3 : 4);
  int clock_width = bounds.size.w >= 160 ? 52 : 32;
  if(bounds.size.w < 160) {
    // Keep the watch's 12/24-hour choice, omitting AM/PM on narrow displays.
    while(clock_text[0] == ' ') memmove(clock_text, clock_text + 1, strlen(clock_text));
    char *suffix = strrchr(clock_text, ' ');
    if(suffix) *suffix = '\0';
  }
  int side_width = (bounds.size.w - 2 * padding - clock_width) / 2;
  graphics_draw_text(ctx, score, fonts_get_system_font(FONT_KEY_GOTHIC_14),
    GRect(padding, 0, side_width, SCORE_HEIGHT), GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
  graphics_draw_text(ctx, clock_text, fonts_get_system_font(FONT_KEY_GOTHIC_14),
    GRect(padding + side_width, 0, clock_width, SCORE_HEIGHT), GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
  graphics_draw_text(ctx, high, fonts_get_system_font(FONT_KEY_GOTHIC_14),
    GRect(bounds.size.w - padding - side_width, 0, side_width, SCORE_HEIGHT),
    GTextOverflowModeTrailingEllipsis, GTextAlignmentRight, NULL);
#ifndef PBL_ROUND
  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_draw_line(ctx, GPoint(0, PLAYFIELD_TOP - 1), GPoint(bounds.size.w - 1, PLAYFIELD_TOP - 1));
#endif
}

static void clock_tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  layer_mark_dirty(s_game_layer);
}

static void draw_game(Layer *layer, GContext *ctx) {
  GRect b = layer_get_bounds(layer);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, b, 0, GCornerNone);
  draw_header(ctx, b);
  GRect apple = cell_rect(s_apple.x, s_apple.y);
  graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(GColorRed, GColorWhite));
  graphics_fill_rect(ctx, apple, 0, GCornerNone);
#ifdef PBL_BW
  // A hollow apple stays distinct from the solid snake on monochrome watches.
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, GRect(apple.origin.x + 1, apple.origin.y + 1,
    apple.size.w - 2, apple.size.h - 2), 0, GCornerNone);
#endif
  graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(GColorGreen, GColorWhite));
  for(int i = 0; i < s_snake_len; i++) {
    graphics_fill_rect(ctx, cell_rect(s_snake[i].x, s_snake[i].y), 0, GCornerNone);
  }
#ifdef PBL_ROUND
  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_draw_rect(ctx, GRect(0, PLAYFIELD_TOP - 1,
    s_board_width + 2, s_board_height + 2));
#endif
}
static void draw_light_control(GContext *ctx, GRect bounds) {
  int y = bounds.size.h - 22;
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, GRect(2, y, 34, 20), 0, GCornerNone);
  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_context_set_fill_color(ctx, GColorWhite);
  if(s_light_keep_on) {
    graphics_fill_circle(ctx, GPoint(10, y + 7), 4);
    graphics_draw_line(ctx, GPoint(10, y), GPoint(10, y + 1));
    graphics_draw_line(ctx, GPoint(3, y + 7), GPoint(4, y + 7));
    graphics_draw_line(ctx, GPoint(16, y + 7), GPoint(17, y + 7));
  } else {
    graphics_draw_circle(ctx, GPoint(10, y + 7), 4);
  }
  graphics_draw_line(ctx, GPoint(8, y + 11), GPoint(8, y + 13));
  graphics_draw_line(ctx, GPoint(12, y + 11), GPoint(12, y + 13));
  graphics_draw_line(ctx, GPoint(8, y + 13), GPoint(12, y + 13));
  graphics_draw_line(ctx, GPoint(9, y + 15), GPoint(11, y + 15));
  graphics_draw_line(ctx, GPoint(27, y + 5), GPoint(27, y + 13));
  graphics_draw_line(ctx, GPoint(24, y + 10), GPoint(27, y + 13));
  graphics_draw_line(ctx, GPoint(27, y + 13), GPoint(30, y + 10));
}

static void draw_ui(Layer *layer, GContext *ctx) {
  GRect b = layer_get_bounds(layer);
  graphics_context_set_text_color(ctx, GColorWhite);
  if(!s_running && !s_game_over && s_score == 0) {
    graphics_draw_text(ctx, "SNAK3\nOK START\nUP/DOWN TURN", fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
      GRect(0, 40, b.size.w, 100), GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
  }
  if(s_paused) {
    graphics_draw_text(ctx, "PAUSE", fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD),
      GRect(0, (b.size.h/2)-20, b.size.w, 40), GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
  }
  if(s_game_over) {
    if(s_blink) {
      graphics_draw_text(ctx, "GAME OVER", fonts_get_system_font(
        PBL_IF_ROUND_ELSE(FONT_KEY_GOTHIC_24_BOLD, FONT_KEY_GOTHIC_28_BOLD)),
        GRect(0, (b.size.h/2)-20, b.size.w, 40), GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
    }
  }
  if(light_control_visible()) draw_light_control(ctx, b);
}
static void select_click_handler(ClickRecognizerRef r, void *c) {
  if(!s_select_pressed) return;
  s_select_pressed = false;
  if(s_select_long_handled) return;
  if(s_game_over) {
    reset_game(); s_running = true; apply_light_mode();
    layer_mark_dirty(s_ui_layer);
    return;
  }
  if(!s_running) { s_running = true; } else { s_paused = !s_paused; }
  apply_light_mode();
  layer_mark_dirty(s_ui_layer);
}
static void select_press_handler(ClickRecognizerRef r, void *c) {
  s_select_pressed = true;
  s_select_long_handled = false;
}
static void select_long_click_handler(ClickRecognizerRef r, void *c) {
  s_select_long_handled = true;
  reset_game();
}
static void up_click_handler(ClickRecognizerRef r, void *c) {
  if(!s_running || s_paused || s_game_over || s_turn_queued) return;
  s_dir = (s_dir + 3) % 4;
  s_turn_queued = true;
}
static void down_click_handler(ClickRecognizerRef r, void *c) {
  if(light_control_visible()) {
    s_light_keep_on = !s_light_keep_on;
    persist_write_int(LIGHT_SETTING_KEY, s_light_keep_on ? 1 : 0);
    apply_light_mode();
    layer_mark_dirty(s_ui_layer);
    return;
  }
  if(!s_running || s_paused || s_game_over || s_turn_queued) return;
  s_dir = (s_dir + 1) % 4;
  s_turn_queued = true;
}
static void click_config_provider(void *c) {
  // Handle short presses on release without also starting after a long reset.
  window_raw_click_subscribe(BUTTON_ID_SELECT, select_press_handler, select_click_handler, NULL);
  window_long_click_subscribe(BUTTON_ID_SELECT, 700, select_long_click_handler, NULL);
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
}
static GRect game_frame(GRect bounds) {
#ifdef PBL_ROUND
  // A square with side <= diameter / sqrt(2) fits entirely inside the display.
  int diameter = bounds.size.w < bounds.size.h ? bounds.size.w : bounds.size.h;
  int side = diameter * 70 / 100;
  bounds.origin.x += (bounds.size.w - side) / 2;
  bounds.origin.y += (bounds.size.h - side) / 2;
  bounds.size.w = side;
  bounds.size.h = side;
#endif
  return bounds;
}
static void window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  GRect b = game_frame(layer_get_bounds(root));
  configure_board(b.size);
  s_game_layer = layer_create(b);
  layer_set_update_proc(s_game_layer, draw_game);
  layer_add_child(root, s_game_layer);
  s_ui_layer = layer_create(b);
  layer_set_update_proc(s_ui_layer, draw_ui);
  layer_add_child(root, s_ui_layer);
  load_highscore();
  load_light_setting();
  s_has_focus = true;
  reset_game();
  app_focus_service_subscribe(focus_handler);
  tick_timer_service_subscribe(MINUTE_UNIT, clock_tick_handler);
}
static void window_unload(Window *window) {
  app_focus_service_unsubscribe();
  light_enable(false);
  s_light_override = false;
  s_has_focus = false;
  tick_timer_service_unsubscribe();
  if(s_timer) {
    app_timer_cancel(s_timer);
    s_timer = NULL;
  }
  layer_destroy(s_ui_layer);
  s_ui_layer = NULL;
  layer_destroy(s_game_layer);
  s_game_layer = NULL;
}
static void init(void) {
  srand(time(NULL));
  s_window = window_create();
  window_set_background_color(s_window, GColorBlack);
  window_set_click_config_provider(s_window, click_config_provider);
  window_set_window_handlers(s_window, (WindowHandlers){
    .load = window_load,
    .unload = window_unload
  });
  window_stack_push(s_window, true);
  s_timer = app_timer_register(180, timer_callback, NULL);
}
static void deinit(void) { window_destroy(s_window); }
int main(void) { init(); app_event_loop(); deinit(); }
