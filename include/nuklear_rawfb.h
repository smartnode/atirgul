/*
 * MIT License
 *
 * Copyright (c) 2016-2017 Patrick Rudolph <siro@das-labor.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
*/
/*
 * ==============================================================
 *
 *                              API
 *
 * ===============================================================
 */

#ifndef NK_RAWFB_H_
#define NK_RAWFB_H_

#include <nuklear.h>

#ifdef __cplusplus
extern "C" {
#endif

struct rawfb_context;

struct rawfb_pl {
    unsigned char bytesPerPixel;
    unsigned char rshift, gshift, bshift, ashift;
    unsigned char rloss, gloss, bloss, aloss;
};

/* All functions are thread-safe */
NK_API struct rawfb_context *nk_rawfb_init(void *fb, void *tex_mem, const unsigned int w, const unsigned int h, const unsigned int pitch, const struct rawfb_pl pl);
NK_API void                  nk_rawfb_render(const struct rawfb_context *rawfb, const struct nk_color clear, const unsigned char enable_clear);
NK_API void                  nk_rawfb_shutdown(struct rawfb_context *rawfb);
NK_API void                  nk_rawfb_resize_fb(struct rawfb_context *rawfb, void *fb, const unsigned int w, const unsigned int h, const unsigned int pitch, const struct rawfb_pl pl);

#ifdef __cplusplus
}
#endif

#endif
