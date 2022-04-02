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


int main() {
  uint8_t *img = calloc(32 * 32, 4);
  FILE *f = fopen("arrow.bin", "r");
  if (f)
    fread(img, 32 * 32, 4, f),
    printf("%d\n", img[0]);
  else
    perror("no can file");

  struct mfb_window *window = mfb_open_ex("sorry leo", WIDTH, HEIGHT, 0);
  if (!window) return 0;

  buf = (uint32_t *) malloc(WIDTH * HEIGHT * 4);
  mfb_set_viewport(window, 0, 0, WIDTH, HEIGHT);

  Mat3 m = mat3_IDENT;
  mat3_translate(m, m, -9.0f, -9.0f);

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
        float a = (float)rgba[3] / 255.0f;
        buf[x*WIDTH + y] = MFB_RGB(
          (int) (a * rgba[0]),
          (int) (a * rgba[1]),
          (int) (a * rgba[2]) + (int) ((1.0f - a) * 55)
        );
      }
      else
        buf[x*WIDTH + y] = MFB_RGB(0, 0, 55); 
    }

  mfb_update_state state;
  do {
    state = mfb_update_ex(window, buf, WIDTH, HEIGHT);
    if (state != STATE_OK) {
      window = NULL;
      break;
    }
  } while(mfb_wait_sync(window));

  return 0;
}
