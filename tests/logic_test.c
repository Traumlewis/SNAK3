#include <assert.h>
#include <stdlib.h>
#include <string.h>
int test_timer_cancels, test_layer_destroys;
static int forced[4], forced_index = 4;
static int fake_rand(void) {
  return forced_index < 4 ? forced[forced_index++] : rand();
}
#define rand fake_rand
#define main watch_main
#include "../src/c/main.c"
#undef main
#undef rand

static void setup(void) {
  s_grid_w=24; s_grid_h=25;
  s_running=true; s_paused=false; s_game_over=false; s_turn_queued=false;
  s_score=0; s_highscore=100; s_speed=180;
  s_snake_len=4; s_dir=DIR_RIGHT;
  s_snake[0]=(Point){5,5}; s_snake[1]=(Point){4,5};
  s_snake[2]=(Point){3,5}; s_snake[3]=(Point){2,5};
  s_apple=(Point){20,20};
}
static void assert_apple_free(void) {
  assert(s_apple.x>=0 && s_apple.x<s_grid_w);
  assert(s_apple.y>=0 && s_apple.y<s_grid_h);
  for(int i=0;i<s_snake_len;i++)
    assert(s_snake[i].x!=s_apple.x || s_snake[i].y!=s_apple.y);
}
int main(void) {
  setup(); s_apple=(Point){6,5};
  int choices[]={6,5,8,8}; memcpy(forced,choices,sizeof(forced)); forced_index=0;
  move_snake();
  assert(s_score==1 && s_snake_len==5);
  assert(s_snake[0].x==6 && s_snake[0].y==5);
  assert(s_snake[4].x==2 && s_snake[4].y==5);
  assert(s_apple.x==8 && s_apple.y==8); assert_apple_free();
  puts("PASS apple cannot spawn in the new head; growth preserves the tail");

  setup();
  s_snake[0]=(Point){2,2}; s_snake[1]=(Point){2,3};
  s_snake[2]=(Point){1,3}; s_snake[3]=(Point){1,2};
  s_dir=DIR_LEFT; move_snake();
  assert(!s_game_over && s_snake[0].x==1 && s_snake[0].y==2);
  setup(); s_dir=DIR_LEFT; move_snake(); assert(s_game_over);
  puts("PASS vacated tail is traversable; occupied body still collides");

  setup(); up_click_handler(NULL,NULL); up_click_handler(NULL,NULL); down_click_handler(NULL,NULL);
  assert(s_dir==DIR_UP); move_snake();
  assert(!s_game_over && s_snake[0].x==5 && s_snake[0].y==4);
  down_click_handler(NULL,NULL); assert(s_dir==DIR_RIGHT);
  s_paused=true; up_click_handler(NULL,NULL); assert(s_dir==DIR_RIGHT);
  puts("PASS rapid clicks cannot reverse direction; next tick accepts another turn");

  setup(); s_running=false; s_select_pressed=false;
  select_click_handler(NULL,NULL); assert(!s_running);
  select_press_handler(NULL,NULL);
  select_long_click_handler(NULL,NULL); select_click_handler(NULL,NULL);
  assert(!s_running && !s_paused && s_score==0);
  select_press_handler(NULL,NULL); select_click_handler(NULL,NULL);
  assert(s_running);
  puts("PASS long reset stays on the title screen; the next short press starts");

  setup();
  s_snake_len=MAX_LEN;
  for(int i=0;i<MAX_LEN;i++) s_snake[i]=(Point){i%24,10+i/24};
  s_snake[0]=(Point){5,5}; s_dir=DIR_RIGHT; s_apple=(Point){6,5};
  for(int i=0;i<500;i++) {
    s_apple=(Point){s_snake[0].x+1,s_snake[0].y};
    if(s_apple.x>=24) { s_snake[0]=(Point){5,5}; s_apple=(Point){6,5}; }
    move_snake();
    if(s_game_over) break;
    assert(s_snake_len==MAX_LEN); assert_apple_free();
  }
  assert(s_score>0);
  puts("PASS eating at maximum length stays within the snake array");

  Window *w=window_create(); window_load(w);
  GRect frame=game_frame(w->root.bounds);
  assert(PLAYFIELD_TOP+s_grid_h*s_cell_size+BOARD_INSET<=frame.size.h);
  assert(2*BOARD_INSET+s_grid_w*s_cell_size<=frame.size.w);
#ifndef PBL_ROUND
  assert(s_grid_h==25 && s_cell_size==8);
#endif
  assert(test_tick_subscribes==1 && test_tick_units==MINUTE_UNIT);
  int dirty_before=test_dirty_count;
  clock_tick_handler(NULL,MINUTE_UNIT);
  assert(test_dirty_count==dirty_before+1 && test_dirty_layer==s_game_layer);

  s_timer=app_timer_register(180,timer_callback,NULL);
  window_unload(w);
  assert(test_timer_cancels==1 && test_layer_destroys==2);
  assert(!s_timer && !s_game_layer && !s_ui_layer);
  assert(test_tick_unsubscribes==1);
  assert(test_focus_subscribes==1 && test_focus_unsubscribes==1 && !test_light_on);
  window_destroy(w);
  puts("PASS playfield fits; window unload cancels timer and frees both layers");

  int sizes[][2]={{144,168},{180,180},{200,228},{260,260}};
  for(unsigned i=0;i<sizeof(sizes)/sizeof(sizes[0]);i++) {
    int width=sizes[i][0],height=sizes[i][1];
    GRect f=game_frame(GRect(0,0,width,height));
    assert(f.origin.x>=0 && f.origin.y>=0);
    assert(f.origin.x+f.size.w<=width && f.origin.y+f.size.h<=height);
#ifdef PBL_ROUND
    int diameter=width<height ? width : height;
    for(int x=0;x<=1;x++) for(int y=0;y<=1;y++) {
      int dx=2*(f.origin.x+x*f.size.w)-width;
      int dy=2*(f.origin.y+y*f.size.h)-height;
      assert(dx*dx+dy*dy<=diameter*diameter);
    }
#else
    assert(f.origin.x==0 && f.origin.y==0 && f.size.w==width && f.size.h==height);
#endif
    configure_board(f.size);
    assert(s_grid_w*s_grid_h>MAX_LEN);
    GRect first=cell_rect(0,0);
    GRect last=cell_rect(s_grid_w-1,s_grid_h-1);
    assert(first.origin.x==BOARD_INSET && first.origin.y==PLAYFIELD_TOP);
    assert(last.origin.x+last.size.w==f.size.w-BOARD_INSET);
    assert(last.origin.y+last.size.h==f.size.h-BOARD_INSET);
    for(int x=1;x<s_grid_w;x++) {
      GRect a=cell_rect(x-1,0),b=cell_rect(x,0);
      assert(a.origin.x+a.size.w==b.origin.x && b.size.w>=3);
    }
    for(int y=1;y<s_grid_h;y++) {
      GRect a=cell_rect(0,y-1),b=cell_rect(0,y);
      assert(a.origin.y+a.size.h==b.origin.y && b.size.h>=3);
    }
    // Reaching the last visible row is legal; the following step hits the wall.
    s_snake_len=4; s_dir=DIR_DOWN; s_game_over=false; s_running=true;
    for(int j=0;j<4;j++) s_snake[j]=(Point){5,s_grid_h-2-j};
    s_apple=(Point){0,0};
    move_snake();
    assert(!s_game_over && s_snake[0].y==s_grid_h-1);
    GRect head=cell_rect(s_snake[0].x,s_snake[0].y);
    assert(head.origin.y+head.size.h==f.size.h-BOARD_INSET);
    move_snake(); assert(s_game_over && s_snake[0].y==s_grid_h-1);
  }
  puts("PASS every display: grid covers all pixels to bottom/right edges; wall collision after the last row");

  int header_widths[]={126,144,182,200};
  for(unsigned i=0;i<sizeof(header_widths)/sizeof(header_widths[0]);i++) {
    int width=header_widths[i];
    s_score=22; s_highscore=34; test_text_count=0; test_line_count=0;
    test_clock_value="13:47";
    draw_header(NULL,GRect(0,0,width,228));
    assert(test_text_count==3);
    assert(strcmp(test_text[0].text,width<140 ? "Score22" : "Score 22")==0);
    assert(test_text[0].align==GTextAlignmentLeft && test_text[0].rect.origin.x>=3);
    assert(strcmp(test_text[1].text,"13:47")==0 && test_text[1].align==GTextAlignmentCenter);
    assert(test_text[1].rect.origin.x*2+test_text[1].rect.size.w==width);
    assert(strcmp(test_text[2].text,width<140 ? "High34" : "High 34")==0);
    assert(test_text[2].align==GTextAlignmentRight);
    assert(test_text[2].rect.origin.x+test_text[2].rect.size.w<=width-3);
    assert(test_text[0].rect.origin.x+test_text[0].rect.size.w<=test_text[1].rect.origin.x);
    assert(test_text[1].rect.origin.x+test_text[1].rect.size.w<=test_text[2].rect.origin.x);
    if(width==200) {
      assert(test_text[0].rect.origin.x==6);
      assert(test_text[2].rect.origin.x+test_text[2].rect.size.w==194);
    }
#ifdef PBL_ROUND
    assert(test_line_count==0);
#else
    assert(test_line_count==1 && test_line_start.x==0 && test_line_end.x==width-1);
    assert(test_line_start.y==PLAYFIELD_TOP-1 && test_line_end.y==PLAYFIELD_TOP-1);
#endif
    test_clock_value="1:47 PM"; test_text_count=0;
    draw_header(NULL,GRect(0,0,width,228));
    assert(strcmp(test_text[1].text,width<160 ? "1:47" : "1:47 PM")==0);
    test_clock_value=" 1:47 PM"; test_text_count=0;
    draw_header(NULL,GRect(0,0,width,228));
    assert(strcmp(test_text[1].text,width<160 ? "1:47" : " 1:47 PM")==0);
  }
  puts("PASS all headers: edge padding, centered 12/24h clock, separated scores; round displays omit extra divider");

  setup(); s_running=false; s_has_focus=true; s_light_override=false;
  test_light_calls=0; test_light_on=false;
  test_persist_has[LIGHT_SETTING_KEY]=false;
  load_light_setting(); assert(!s_light_keep_on);
  int old_high=test_persist_values[1];
  Direction before=s_dir;
  down_click_handler(NULL,NULL);
  assert(s_light_keep_on && s_dir==before && !test_light_on);
  assert(test_persist_last_key==LIGHT_SETTING_KEY && test_persist_values[LIGHT_SETTING_KEY]==1);
  select_press_handler(NULL,NULL); select_click_handler(NULL,NULL);
  assert(s_running && test_light_on && s_light_override);
  int light_calls=test_light_calls;
  apply_light_mode(); assert(test_light_calls==light_calls);
  down_click_handler(NULL,NULL);
  assert(s_dir==DIR_DOWN && s_light_keep_on && test_light_on);
  select_press_handler(NULL,NULL); select_click_handler(NULL,NULL);
  assert(s_paused && !test_light_on);
  before=s_dir;
  down_click_handler(NULL,NULL);
  assert(!s_light_keep_on && s_dir==before && !test_light_on);
  assert(test_persist_values[LIGHT_SETTING_KEY]==0 && test_persist_values[1]==old_high);
  select_press_handler(NULL,NULL); select_click_handler(NULL,NULL);
  assert(!s_paused && !test_light_on);
  puts("PASS light controls: DOWN changes mode only in menus; gameplay turns normally; pause releases forced light");

  select_press_handler(NULL,NULL); select_click_handler(NULL,NULL);
  down_click_handler(NULL,NULL);
  select_press_handler(NULL,NULL); select_click_handler(NULL,NULL);
  assert(s_light_keep_on && test_light_on);
  focus_handler(false); assert(!test_light_on);
  focus_handler(true); assert(test_light_on);
  game_over(); assert(!test_light_on);
  int writes=test_persist_writes;
  down_click_handler(NULL,NULL); assert(test_persist_writes==writes && s_light_keep_on);
  select_press_handler(NULL,NULL); select_click_handler(NULL,NULL);
  assert(s_running && test_light_on && s_light_keep_on);
  reset_game(); assert(!test_light_on && s_light_keep_on);
  s_light_keep_on=false; load_light_setting(); assert(s_light_keep_on);
  test_persist_values[LIGHT_SETTING_KEY]=7;
  load_light_setting(); assert(!s_light_keep_on);
  puts("PASS light lifecycle: focus loss, game over and reset release light; valid preference survives restart; invalid values fall back to SYSTEM");

  Layer ui={.bounds=GRect(0,0,200,228)};
  s_light_keep_on=false; test_draw_circles=0; test_fill_circles=0;
  draw_ui(&ui,NULL); assert(test_draw_circles==1 && test_fill_circles==0);
  s_light_keep_on=true; test_draw_circles=0; test_fill_circles=0;
  draw_ui(&ui,NULL); assert(test_draw_circles==0 && test_fill_circles==1);
  s_running=true; s_paused=false; test_fill_circles=0;
  draw_ui(&ui,NULL); assert(test_fill_circles==0);
  s_paused=true; draw_ui(&ui,NULL); assert(test_fill_circles==1);
  s_paused=false; s_game_over=true; test_fill_circles=0;
  draw_ui(&ui,NULL); assert(test_fill_circles==0);
  puts("PASS bulb icon: outlined SYSTEM/filled ON in start and pause, hidden during gameplay and game over");
  return 0;
}
