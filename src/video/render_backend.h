#ifndef RENDER_BACKEND_H
#define RENDER_BACKEND_H

#include <stdint.h>

struct window;

typedef int texture_handle_t;

enum pxl_format {
  PXL_INVALID,
  PXL_RGBA,
  PXL_RGBA5551,
  PXL_RGB565,
  PXL_RGBA4444,
  PXL_RGBA8888,
};

enum filter_mode {
  FILTER_NEAREST,
  FILTER_BILINEAR,
  NUM_FILTER_MODES,
};

enum wrap_mode {
  WRAP_REPEAT,
  WRAP_CLAMP_TO_EDGE,
  WRAP_MIRRORED_REPEAT,
};

enum depth_func {
  DEPTH_NONE,
  DEPTH_NEVER,
  DEPTH_LESS,
  DEPTH_EQUAL,
  DEPTH_LEQUAL,
  DEPTH_GREATER,
  DEPTH_NEQUAL,
  DEPTH_GEQUAL,
  DEPTH_ALWAYS,
};

enum cull_face {
  CULL_NONE,
  CULL_FRONT,
  CULL_BACK,
};

enum blend_func {
  BLEND_NONE,
  BLEND_ZERO,
  BLEND_ONE,
  BLEND_SRC_COLOR,
  BLEND_ONE_MINUS_SRC_COLOR,
  BLEND_SRC_ALPHA,
  BLEND_ONE_MINUS_SRC_ALPHA,
  BLEND_DST_ALPHA,
  BLEND_ONE_MINUS_DST_ALPHA,
  BLEND_DST_COLOR,
  BLEND_ONE_MINUS_DST_COLOR,
};

enum shade_mode {
  SHADE_DECAL,
  SHADE_MODULATE,
  SHADE_DECAL_ALPHA,
  SHADE_MODULATE_ALPHA,
};

enum box_type {
  BOX_BAR,
  BOX_FLAT,
};

enum prim_type {
  PRIM_TRIANGLES,
  PRIM_LINES,
};

struct vertex {
  float xyz[3];
  float uv[2];
  uint32_t color;
  uint32_t offset_color;
};

struct surface {
  texture_handle_t texture;
  int depth_write;
  enum depth_func depth_func;
  enum cull_face cull;
  enum blend_func src_blend;
  enum blend_func dst_blend;

  enum shade_mode shade;
  int ignore_alpha;
  int ignore_texture_alpha;
  int offset_color;
  int pt_alpha_test;
  float pt_alpha_ref;

  int first_vert;
  int num_verts;
};

struct vertex2d {
  float xy[2];
  float uv[2];
  uint32_t color;
};

struct surface2d {
  enum prim_type prim_type;
  texture_handle_t texture;
  enum blend_func src_blend;
  enum blend_func dst_blend;
  int scissor;
  float scissor_rect[4];
  int first_vert;
  int num_verts;
};

struct render_backend;

struct render_backend *rb_create(struct window *window);
void rb_destroy(struct render_backend *rb);

texture_handle_t rb_create_texture(struct render_backend *rb,
                                   enum pxl_format format,
                                   enum filter_mode filter,
                                   enum wrap_mode wrap_u, enum wrap_mode wrap_v,
                                   int mipmaps, int width, int height,
                                   const uint8_t *buffer);
void rb_destroy_texture(struct render_backend *rb, texture_handle_t handle);

void rb_begin_frame(struct render_backend *rb);
void rb_end_frame(struct render_backend *rb);

void rb_begin_ortho(struct render_backend *rb);
void rb_end_ortho(struct render_backend *rb);

void rb_begin_surfaces(struct render_backend *rb, const float *projection,
                       const struct vertex *verts, int num_verts);
void rb_draw_surface(struct render_backend *rb, const struct surface *surf);
void rb_end_surfaces(struct render_backend *rb);

void rb_begin_surfaces2d(struct render_backend *rb,
                         const struct vertex2d *verts, int num_verts,
                         uint16_t *indices, int num_indices);
void rb_draw_surface2d(struct render_backend *rb, const struct surface2d *surf);
void rb_end_surfaces2d(struct render_backend *rb);

#endif
