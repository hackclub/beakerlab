#include "jerry_install/include/jerryscript.h"
#include "minifb/include/MiniFB.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <math.h>

#define WIDTH (320)
#define HEIGHT (320)
static uint32_t *buf = 0x0;

typedef struct { float x, y, z; } Vec3;
typedef float Mat3[9];
static void mat3_transform_vec3(Vec3 *out, Vec3 a, Mat3 m) {
  out->x = a.x * m[0] + a.y * m[3] + a.z * m[6];
  out->y = a.x * m[1] + a.y * m[4] + a.z * m[7];
  out->z = a.x * m[2] + a.y * m[5] + a.z * m[8];
}

static void mat3_rotate(Mat3 out, Mat3 a, float rad) {
  /* caching these here to support out == a */
  float a00 = a[0], a01 = a[1], a02 = a[2],
        a10 = a[3], a11 = a[4], a12 = a[5],
        a20 = a[6], a21 = a[7], a22 = a[8],
        s = sinf(rad), c = cosf(rad);
  out[0] = c * a00 + s * a10;
  out[1] = c * a01 + s * a11;
  out[2] = c * a02 + s * a12;
  out[3] = c * a10 - s * a00;
  out[4] = c * a11 - s * a01;
  out[5] = c * a12 - s * a02;
  out[6] = a20;
  out[7] = a21;
  out[8] = a22;
}

static void mat3_translate(Mat3 out, Mat3 a, float x, float y) {
  out[6] = a[6] + x;
  out[7] = a[7] + y;
}

static void mat3_scale(Mat3 out, Mat3 a, float x, float y) {
  out[0] = a[0] * x;
  out[4] = a[4] * y;
}

static void mat3_mul(Mat3 out, Mat3 a, Mat3 b) {
  /* cache so out can be a or b */
  float a00 = a[0], a01 = a[1], a02 = a[2];
  float a10 = a[3], a11 = a[4], a12 = a[5];
  float a20 = a[6], a21 = a[7], a22 = a[8];
  float b00 = b[0], b01 = b[1], b02 = b[2];
  float b10 = b[3], b11 = b[4], b12 = b[5];
  float b20 = b[6], b21 = b[7], b22 = b[8];
  out[0] = b00 * a00 + b01 * a10 + b02 * a20;
  out[1] = b00 * a01 + b01 * a11 + b02 * a21;
  out[2] = b00 * a02 + b01 * a12 + b02 * a22;
  out[3] = b10 * a00 + b11 * a10 + b12 * a20;
  out[4] = b10 * a01 + b11 * a11 + b12 * a21;
  out[5] = b10 * a02 + b11 * a12 + b12 * a22;
  out[6] = b20 * a00 + b21 * a10 + b22 * a20;
  out[7] = b20 * a01 + b21 * a11 + b22 * a21;
  out[8] = b20 * a02 + b21 * a12 + b22 * a22;
}

#define mat3_IDENT ((Mat3) { 1.0f, 0.0f, 0.0f,\
                             0.0f, 1.0f, 0.0f,\
                             0.0f, 0.0f, 1.0f})

typedef uint8_t Image[32 * 32 * 4];
typedef enum {
  ImageKind_NONE,
  ImageKind_Arrow,
  ImageKind_Archer,
  ImageKind_COUNT,
} ImageKind;
static void image_load(char *path, uint8_t *img) {
  FILE *f = fopen(path, "r");
  if (f)
    fread(img, 32 * 32, 4, f),
    printf("%d\n", img[0]);
  else
    perror("no can file");
}

typedef struct { float x, y, image; } Sprite;

#define SPRITE_COUNT (2)
static struct {
  Image images[ImageKind_COUNT];
} rend;

static void render_init(void) {
  image_load("../arrow.bin", (uint8_t *) (rend.images + ImageKind_Arrow));
  image_load("../archer.bin", (uint8_t *) (rend.images + ImageKind_Archer));
}

static void render_clear(uint32_t *buf) {
  for (int x = 0; x < WIDTH; x++)
    for (int y = 0; y < HEIGHT; y++)
      buf[x*WIDTH + y] = MFB_RGB(30, 30, 85); 
}

static void render_sprite(uint32_t *buf, Sprite *sprite) {
  uint8_t *img = rend.images[(size_t) sprite->image];

  Mat3 m = mat3_IDENT;
  mat3_translate(m, m, sprite->x, sprite->y);

  Mat3 r = mat3_IDENT;
  mat3_rotate(r, r, M_PI/4.0f);

  mat3_mul(m, r, m);

  Vec3 p = { 0, 0, 1 }, q;
  for (int x = 0; x < WIDTH; x++)
    for (int y = 0; y < HEIGHT; y++) {
      p.x = x, p.y = y;
      mat3_transform_vec3(&q, p, m);

      if (q.x >= 0.0f && q.x <= 32.0f && 
          q.y >= 0.0f && q.y <= 32.0f ) {
        int u = q.x, v = q.y;
        uint8_t *rgba = img + (u*32 + v) * 4;
        if (rgba[3] == 0) continue;
        float a = (float)rgba[3] / 255.0f;
        buf[x*WIDTH + y] = MFB_RGB(
          (int) (a * rgba[0]),
          (int) (a * rgba[1]),
          (int) (a * rgba[2]) + (int) ((1.0f - a) * 55)
        );
      }
    }
}

static void render(uint32_t *buf, Sprite *sprites) {
  render_clear(buf);

  for (Sprite *s = sprites; (s - sprites) < SPRITE_COUNT; s++)
    if ((size_t) s->image != ImageKind_NONE)
      render_sprite(buf, s);
}

static struct {
  jerry_value_t glob; /* global object */
  jerry_value_t name_dt;

  jerry_value_t anim_handler;
  jerry_value_t spritearraybuffer;
  Sprite *spritebuf;
} js;

static jerry_value_t js_setAnimHandler(
  const jerry_call_info_t *call_info_p,
  const jerry_value_t args[],
  const jerry_length_t n_arg
) {
  if (n_arg < 1 || !jerry_value_is_function(args[0]))
    return jerry_undefined();

  js.anim_handler = jerry_value_copy(args[0]);

  return jerry_undefined();
}

static void js_add_global_func(char *name, jerry_external_handler_t func) {
  jerry_value_t property_name_print = jerry_string_sz(name);
  jerry_value_t property_value_func =
    jerry_function_external(func);
  jerry_value_t set_result = jerry_object_set(
    js.glob,
    property_name_print,
    property_value_func
  );

  if (jerry_value_is_exception(set_result))
    puts("Failed to add the 'print' property");

  jerry_value_free(set_result);
  jerry_value_free(property_value_func);
  jerry_value_free(property_name_print);
}

static void js_log(jerry_value_t val) {
  jerry_value_t string_value = jerry_value_to_string(val);

  jerry_char_t buffer[256] = "too big to print!";

  jerry_size_t copied_bytes = jerry_string_to_buffer(
    string_value,
    JERRY_ENCODING_UTF8,
    buffer,
    sizeof(buffer) - 1
  );
  buffer[copied_bytes] = '\0';

  jerry_value_free(string_value);

  puts((char *) buffer);
}

static void js_eat_err(jerry_value_t err) {
  if (jerry_value_is_exception(err))
    puts("js error: "), js_log(jerry_exception_value(err, false));

  jerry_value_free(err);
}

static void js_init(void) {
  jerry_init(JERRY_INIT_EMPTY);

  js.glob = jerry_current_realm();
  js.name_dt = jerry_string_sz("dt");
  js.anim_handler = jerry_undefined();

  js.spritearraybuffer = jerry_arraybuffer(sizeof(Sprite) * SPRITE_COUNT);
  js.spritebuf = (Sprite *) jerry_arraybuffer_data(js.spritearraybuffer);
  js.spritebuf[0] = (Sprite) { -9.0f, -9.9f };

  /* add spritepush */
  js_add_global_func("setAnimHandler", js_setAnimHandler);

  /* parse */
  const jerry_char_t script[] = "let t = 0;"
                                "setAnimHandler((dt, buf) => {"
                                "  const sprites = new Float32Array(buf);"
                                "  sprites[0] = -18;"
                                "  sprites[1] = (-t * 50) % 200;"
                                "  sprites[2] = 1;"
                                "  sprites[3] = -100 + Math.cos(-t) * 50;"
                                "  sprites[4] = -100 + Math.sin(-t) * 50;"
                                "  sprites[5] = 2;"
                                "  t += dt;"
                                "});";
  const jerry_length_t script_size = sizeof (script) - 1;

  jerry_value_t parsed_code = jerry_parse(script, script_size, NULL);
  if (jerry_value_is_exception(parsed_code))
    puts("error parsing - TODO: more info here");

  js_eat_err(jerry_run(parsed_code));

  jerry_value_free(parsed_code);
}

static void js_deinit(void) {
  jerry_value_free(js.glob);
  jerry_value_free(js.name_dt);
  jerry_value_free(js.anim_handler);
  jerry_value_free(js.spritearraybuffer);

  jerry_cleanup();
}

static void js_run_anim_handler(double dt) {
  if (jerry_value_is_undefined(js.anim_handler)) return;

  jerry_value_t args[] = { jerry_number(dt), js.spritearraybuffer };
  jerry_size_t n_arg = sizeof(args) / sizeof(args[0]);
  js_eat_err(jerry_call(js.anim_handler, jerry_undefined(), args, n_arg));

  jerry_value_free(args[0]);
}

int main() {
  render_init();
  js_init();

  struct mfb_window *window = mfb_open_ex("sorry leo", WIDTH, HEIGHT, 0);
  if (!window) return 0;

  mfb_set_viewport(window, 0, 0, WIDTH, HEIGHT);
  mfb_set_target_fps(60);
  struct mfb_timer *timer = mfb_timer_create();

  mfb_update_state state;
  buf = (uint32_t *) malloc(WIDTH * HEIGHT * 4);
  do {
    state = mfb_update_ex(window, buf, WIDTH, HEIGHT);

    render(buf, js.spritebuf);
    js_run_anim_handler(mfb_timer_delta(timer));

    if (state != STATE_OK) {
      window = NULL;
      break;
    }
  } while(mfb_wait_sync(window));

  mfb_timer_destroy(timer);
  js_deinit();
  free(buf);

  return 0;
}
