/*****************************************************************************
 *
 * Nuklear Cairo Render Backend
 * Copyright 2025 Elmurod Talipov
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 ****************************************************************************/

#ifndef NK_CAIRO_H
#define NK_CAIRO_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    NK_CARIO_ROTATE_0 = 0,
    NK_CAIRO_ROTATE_90 = 90,
    NK_CAIRO_ROTATE_180 = 180,
    NK_CAIRO_ROTATE_270 = 270
} nk_cairo_rotate_e;

struct nk_context;
struct nk_cairo_context;
struct nk_cairo_font;

NK_API struct nk_cairo_context *nk_cairo_init(uint8_t *bufer, int width, int height, int bpp, nk_cairo_rotate_e rotate);
NK_API void nk_cairo_deinit(struct nk_cairo_context *cairo_ctx);
NK_API struct nk_context *nk_cairo_get_nk_context(struct nk_cairo_context *cairo_ctx);
NK_API bool nk_cairo_render(struct nk_cairo_context *cairo_ctx);
NK_API void nk_cairo_damage(struct nk_cairo_context *cairo_ctx);
NK_API void nk_cairo_dump_surface(struct nk_cairo_context *cairo_ctx, const char *filename);
NK_API struct nk_cairo_font *nk_cairo_create_font(struct nk_cairo_context *cairo_ctx, const char *font_family, float font_size);
NK_API struct nk_user_font *nk_cairo_get_user_font(struct nk_cairo_font *cairo_font);
NK_API void nk_cairo_destory_font(struct nk_cairo_font *font);

#ifdef __cplusplus
}
#endif

#endif /* NK_CAIRO_H */