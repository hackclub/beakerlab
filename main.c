#include "minifb/include/MiniFB.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <math.h>

#define WIDTH (320)
#define HEIGHT (320)
static uint32_t *buf = 0x0;

char *map_str = 
  "bbbbbbbbbb\n"
  "bp......gb\n"
  "b....b...b\n"
  "b.b#bg.#.b\n"
  "b......b.b\n"
  "bbbbbbbbbb\n";

char *duck_str =
  "................\n"
  "................\n"
  ".......0000.....\n"
  ".......0ggg0....\n"
  "......0ggggg0...\n"
  "......0gggbgr...\n"
  "......0gggggrr..\n"
  ".......0gggg0...\n"
  "..00000ggggg0...\n"
  "..0ggggggggg0...\n"
  "..0ggggggggg0...\n"
  "...0ggggggg0....\n"
  "....0ggggg00....\n"
  ".....00000......\n"
  "......00.00.....\n"
  "................\n";

typedef struct Sprite Sprite;
struct Sprite {
  char type;
  uint8_t x, y, dx, dy;
  Sprite *next;
};

static Sprite *sprite_pool;

static uint32_t char_color[255] = {
  ['b'] = 0x00000000,
  ['0'] = 0x00000000,
  ['#'] = 0x00FF0000,
  ['g'] = 0x0000FF00,
  ['r'] = 0x00FF0000,
  ['p'] = 0xA0AAA0A0,
};
static uint8_t char_solid[255] = {
  ['p'] = 1,
  ['b'] = 1,
  ['#'] = 1,
};
typedef struct { char key, val; } CharPushAssoc;
static CharPushAssoc char_push_assoc[1 << 7];
static int char_push_assoc_count = 0;

static uint8_t sprite_can_push(char key, char val) {
  for (
    CharPushAssoc *cpa = char_push_assoc;
    (cpa - char_push_assoc) < char_push_assoc_count;
    cpa++
  ) {
    if (cpa->key == key && cpa->val == val)
      return true;
  }
  return false;
}

#define MAP_SIZE_X (100)
#define MAP_SIZE_Y (100)
typedef Sprite *SpriteStack;
static SpriteStack map[MAP_SIZE_X][MAP_SIZE_Y];

typedef struct { uint8_t x, y; } MatchState;
static uint8_t map_match(MatchState *ms, char p) {
  Sprite *s;
  for (; ms->y < MAP_SIZE_Y; ms->y++) {
    for (; ms->x < MAP_SIZE_X; ms->x++)
      if ((s = map[ms->x][ms->y]))
        for (; s; s = s->next)
          if (map[ms->x][ms->y]->type == p)
            return 1;
    ms->x = 0;
  }
  return 0;
}


/* does this swap entire tiles or just the relevant sprites? */
/*
static int map_swap(char a, char b) {
  int i = 0;
  MatchState ms = {0};
  while (map_match(&ms, a)) {
    i++;
    Sprite s = (Sprite) { .x = ms.x, .y = ms.y, .type = b };
    if (map[ms.x][ms.y]) *map[ms.x][ms.y] = s;
    else *(map[ms.x][ms.y] = sprite_pool++) = s;
  }
  return i;
}*/

static Sprite *sprite_add(int x, int y, char type) {
  Sprite s = (Sprite) { .x = x, .y = y, .type = type };
  s.next = map[x][y];
  *(map[x][y] = sprite_pool++) = s;
  return map[x][y];
}

/* removes sprite from its tile */
static void sprite_pluck(Sprite *s) {
  Sprite *n = map[s->x][s->y];

  if (n == s){
    map[s->x][s->y] = s->next;
    return;
  }
  for (; n->next; n = n->next)
    if (n->next == s) {
      n->next = s->next;
      return;
    }
}

static uint8_t sprite_move(Sprite *s, int dx, int dy) {
  if (char_solid[(int)s->type]) {
    /* no moving into a solid! */
    Sprite *n = map[s->x+dx][s->y+dy];
    for (; n; n = n->next)
      if (char_solid[(int)n->type]) {
        /* unless you can push them out of the way ig */
        if (sprite_can_push(s->type, n->type)) {
          if (!sprite_move(n, dx, dy))
            return false;
        }
        else
          return false;
      }

    /* ok, what'd we push? */
    n = map[s->x+dx][s->y+dy];
    for (; n; n = n->next)
      if (sprite_can_push(s->type, n->type))
        sprite_move(n, dx, dy);
  }

  sprite_pluck(s);
  s->x += dx;
  s->y += dy;
  s->dx = dx;
  s->dy = dy;
  s->next = map[s->x][s->y];
  map[s->x][s->y] = s;
  return true;
}

static void render_init(void) {
  char_push_assoc[char_push_assoc_count++] = (CharPushAssoc) {
    .key = 'p',
    .val = '#'
  };
  sprite_pool = calloc(sizeof(Sprite), 1 << 11);

  char *str = map_str;
  int tx = 0, ty = 0;
  do {
    switch (*str) {
      case '\n': ty++, tx = 0; break;
      case  '.': tx++;         break;
      case '\0':               break;
      default: {
        map[tx][ty] = sprite_pool++;
        *map[tx][ty] = (Sprite) { .x = tx, .y = ty, .type = *str };
        tx++;
      } break;
    }
  } while (*str++);
}

static void render_tile(uint32_t *buf, uint8_t tx, uint8_t ty) {
  char type = map[tx][ty]->type;
  for (int x = 0; x < 16; x++)
    for (int y = 0; y < 16; y++)
      buf[(y + ty*16)*HEIGHT + (x + tx*16)] = char_color[(int)type]; 
}

void keyboard(
  struct mfb_window *window,
  mfb_key key,
  mfb_key_mod mod,
  bool isPressed
) {
  if (!isPressed) return;

  if (key == KB_KEY_ESCAPE) mfb_close(window);

  int dx = 0, dy = 0;
  if (key == KB_KEY_W) dy --;
  if (key == KB_KEY_S) dy ++;
  if (key == KB_KEY_A) dx --;
  if (key == KB_KEY_D) dx ++;
  
  if (dx == 0 && dy == 0) return;

  MatchState ms = {0};
  if (map_match(&ms, 'p')) {
    sprite_move(map[ms.x][ms.y], dx, dy);
  }
}

static void render_clear(uint32_t *buf) {
  for (int x = 0; x < WIDTH; x++)
    for (int y = 0; y < HEIGHT; y++)
      buf[x*WIDTH + y] = MFB_RGB(30, 30, 85); 

  for (int x = 0; x < MAP_SIZE_X; x++)
    for (int y = 0; y < MAP_SIZE_Y; y++)
      if (map[x][y] && map[x][y]->type)
        render_tile(buf, x, y);
}

static void render(uint32_t *buf) {
  render_clear(buf);
}

int main() {
  render_init();

  struct mfb_window *window = mfb_open_ex("c engine preview", WIDTH, HEIGHT, 0);
  if (!window) return 0;

  mfb_set_viewport(window, 0, 0, WIDTH, HEIGHT);
  mfb_set_target_fps(60);
  struct mfb_timer *timer = mfb_timer_create();

  mfb_set_keyboard_callback(window, keyboard);

  mfb_update_state state;
  buf = (uint32_t *) malloc(WIDTH * HEIGHT * 4);
  do {
    state = mfb_update_ex(window, buf, WIDTH, HEIGHT);

    render(buf);

    if (state != STATE_OK) {
      window = NULL;
      break;
    }
  } while(mfb_wait_sync(window));

  mfb_timer_destroy(timer);
  free(buf);

  return 0;
}
