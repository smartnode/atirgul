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

#include "nuklear.h"
#include "nuklear_cairo.h"

#ifdef NK_CAIRO_IMPLEMENTATION

#define DEFAULT_FONT_SIZE 12.0f
#define DEFAULT_FONT_NAME "Arial"

#include <stdlib.h>
#include <cairo.h>
#include <glib.h>
#include <pango/pangocairo.h>

struct nk_cairo_font {
    PangoContext *pctx;
    PangoFontDescription *desc;
    struct nk_user_font nkufont;
};

struct nk_cairo_context {
    cairo_t *cr;
    cairo_surface_t *surface;
    PangoContext *pango_ctx;

    struct nk_context *nk_ctx;
    struct nk_cairo_font *font;

    void *last_buffer_addr;
    nk_size last_buffer_size;
    int repaint;
};

#define NK_TO_CAIRO(x) ((double) x / 255.0)
#define NK_CAIRO_DEG_TO_RAD(x) ((double) x * NK_PI / 180.0)


NK_INTERN float nk_cairo_text_width(nk_handle handle, float height, const char *text, int len)
{
    ENT();
    struct nk_cairo_font *font = (struct nk_cairo_font*)handle.ptr;
    
    // Create layout for measurement only
    PangoLayout *layout = pango_layout_new(font->pctx);
    pango_layout_set_text(layout, text, len);
    pango_layout_set_font_description(layout, font->desc);
    
    // Measure
    int w = 0, h = 0;
    pango_layout_get_pixel_size(layout, &w, &h);
    g_object_unref(layout);

    EXT();
    return (float)w;
}


NK_API struct nk_cairo_font *nk_cairo_create_font(struct nk_cairo_context *cairo_ctx, const char *font_family, float font_size)
{
    ENT();
    if (cairo_ctx == NULL || cairo_ctx->pango_ctx == NULL || font_family == NULL || font_size < 0.01f) {
        ERR("Invalid parameters");
        return NULL;
    }

    struct nk_cairo_font *font = (struct nk_cairo_font *)calloc(1, sizeof(struct nk_cairo_font));
    if (font == NULL) {
        ERR("Failed to callocate memory for nk cairo font");
        return NULL;
    }

    g_autofree char *desc_str = g_strdup_printf("%s %f", font_family, font_size);
    if (desc_str == NULL) {
        ERR("Failed to alloate memory for font descriptions");
        free(font);
        return NULL;
    }

    font->desc = pango_font_description_from_string(desc_str);
    if (font->desc == NULL) {
        ERR("Failed to allocate font from pango");
        free(font);
        return NULL;
    }
    font->pctx = g_object_ref(cairo_ctx->pango_ctx);
    font->nkufont.userdata = nk_handle_ptr(font);
    font->nkufont.height = font_size;
    font->nkufont.width = nk_cairo_text_width;

    EXT();
    return font;
}

NK_API struct nk_user_font *nk_cairo_get_user_font(struct nk_cairo_font *cairo_font)
{
    ENT();
    if (cairo_font == NULL) {
        ERR("Invalid parameter");
        return NULL;
    }
    EXT();
    return &(cairo_font->nkufont);
}

NK_API void nk_cairo_destory_font(struct nk_cairo_font *font)
{
    ENT();
    if (font) {
        pango_font_description_free(font->desc);
        g_object_unref(font->pctx);
        font->desc = NULL;
        font->nkufont.userdata.ptr = NULL;
        font->nkufont.width = NULL;
        free(font);
    }
    EXT();
}

NK_API struct nk_cairo_context *nk_cairo_init(uint8_t *buffer, int width, int height, int bpp, nk_cairo_rotate_e rotate)
{
    ENT();
    if (buffer == NULL || width <=0 || height <= 0 || bpp <= 0)
    {
        ERR("Invalid parameter");
        return NULL;
    }

    // clear buffer 
    memset(buffer, 0, width * height * bpp);

    struct nk_cairo_context *cairo_ctx = calloc(1, sizeof(struct nk_cairo_context));
    if (cairo_ctx == NULL) {
        ERR("Failed to allocate memory for cairo context");
        return NULL;
    }
    cairo_ctx->last_buffer_addr = NULL;
    cairo_ctx->last_buffer_size = 0;
    cairo_ctx->repaint = nk_false;

    //TODO: swap width/height for 90/270 degree rotation
    int stride = width * bpp;
    cairo_ctx->surface = cairo_image_surface_create_for_data(buffer, CAIRO_FORMAT_ARGB32, width, height, stride);
    if (cairo_ctx->surface == NULL) {
        ERR("Failed to create cairo surface");
        nk_cairo_deinit(cairo_ctx);
        return NULL;
    }

    cairo_ctx->cr = cairo_create(cairo_ctx->surface);
    if (cairo_ctx->cr == NULL) {
        ERR("Failed to create caior");
        nk_cairo_deinit(cairo_ctx);
        return NULL;
    }
    cairo_t *cr = cairo_ctx->cr;

    cairo_ctx->pango_ctx = pango_cairo_create_context(cr);
    if (cairo_ctx->pango_ctx == NULL) {
        ERR("Failed to create pango context");
        nk_cairo_deinit(cairo_ctx);
        return NULL;
    }

    cairo_ctx->font = nk_cairo_create_font(cairo_ctx, DEFAULT_FONT_NAME, DEFAULT_FONT_SIZE);
    if (cairo_ctx->font == NULL) {
        nk_cairo_deinit(cairo_ctx);
        return NULL;
    }

    cairo_ctx->nk_ctx = (struct nk_context *)calloc(1, sizeof(struct nk_context));
    if (cairo_ctx->nk_ctx == NULL) {
        ERR("Failed to allocate memory for nuklear context");
        nk_cairo_deinit(cairo_ctx);
        return NULL;
    }

    nk_bool ret = nk_init_default(cairo_ctx->nk_ctx, &(cairo_ctx->font->nkufont));
    if (ret == nk_false) {
        ERR("Failed to initialize nuklear with nk_init_default");
        nk_cairo_deinit(cairo_ctx);
        return NULL;
    }

    EXT();
    return cairo_ctx;
}

NK_API void nk_cairo_deinit(struct nk_cairo_context *cairo_ctx)
{
    ENT();
    if (cairo_ctx) {
        if (cairo_ctx->last_buffer_addr) {
            free(cairo_ctx->last_buffer_addr);
            cairo_ctx->last_buffer_addr = NULL;
            cairo_ctx->last_buffer_size = 0;
        }

        if (cairo_ctx->nk_ctx) {
            nk_free(cairo_ctx->nk_ctx);
            free(cairo_ctx->nk_ctx);
            cairo_ctx->nk_ctx = NULL;
        }

        if (cairo_ctx->font) {
            nk_cairo_destory_font(cairo_ctx->font);
            cairo_ctx->font = NULL;
        }

        if (cairo_ctx->pango_ctx) {
            g_object_unref(cairo_ctx->pango_ctx);
        }

        cairo_destroy(cairo_ctx->cr);
        cairo_surface_destroy(cairo_ctx->surface);
        free(cairo_ctx);
    }
    EXT();
}

NK_API struct nk_context *nk_cairo_get_nk_context(struct nk_cairo_context *cairo_ctx)
{
    ENT();
    struct nk_context *ctx = cairo_ctx ? cairo_ctx->nk_ctx : NULL;
    EXT();
    return ctx;
}

NK_API void nk_cairo_damage(struct nk_cairo_context *cairo_ctx)
{
    if (cairo_ctx)
       cairo_ctx->repaint = nk_true;
}

NK_API bool nk_cairo_render(struct nk_cairo_context *cairo_ctx)
{
    ENT();
    struct nk_context *nk_ctx = cairo_ctx->nk_ctx;
    const struct nk_command *cmd = NULL;
    void *cmds = nk_buffer_memory(&nk_ctx->memory);

    if (cairo_ctx->last_buffer_size != nk_ctx->memory.allocated) {
        cairo_ctx->last_buffer_size = nk_ctx->memory.allocated;
        cairo_ctx->last_buffer_addr = realloc(cairo_ctx->last_buffer_addr, cairo_ctx->last_buffer_size);
        memcpy(cairo_ctx->last_buffer_addr, cmds, cairo_ctx->last_buffer_size);
    }
    else if (!memcmp(cmds, cairo_ctx->last_buffer_addr, cairo_ctx->last_buffer_size)) {
        if (!cairo_ctx->repaint) {
            nk_clear(nk_ctx);
            return nk_false;
        }
        cairo_ctx->repaint = nk_false;
    }
    else {
        memcpy(cairo_ctx->last_buffer_addr, cmds, cairo_ctx->last_buffer_size);
    }

    cairo_t *cr = cairo_ctx->cr;
    cairo_push_group(cr);

    nk_foreach(cmd, nk_ctx) {
        DBG("Command: %d", cmd->type);
        switch (cmd->type) {
        case NK_COMMAND_NOP:
            break;
        case NK_COMMAND_SCISSOR:
            {
                const struct nk_command_scissor *s = (const struct nk_command_scissor *)cmd;
                cairo_reset_clip(cr);
                if (s->x >= 0) {
                    cairo_rectangle(cr, s->x - 1, s->y - 1, s->w + 2, s->h + 2);
                    cairo_clip(cr);
                }
            }
            break;
        case NK_COMMAND_LINE:
            {
                const struct nk_command_line *l = (const struct nk_command_line *)cmd;
                cairo_set_source_rgba(cr, NK_TO_CAIRO(l->color.r), NK_TO_CAIRO(l->color.g), NK_TO_CAIRO(l->color.b), NK_TO_CAIRO(l->color.a));
                cairo_set_line_width(cr, l->line_thickness);
                cairo_move_to(cr, l->begin.x, l->begin.y);
                cairo_line_to(cr, l->end.x, l->end.y);
                cairo_stroke(cr);
            }
            break;
        case NK_COMMAND_CURVE:
            {
                const struct nk_command_curve *q = (const struct nk_command_curve *)cmd;
                cairo_set_source_rgba(cr, NK_TO_CAIRO(q->color.r), NK_TO_CAIRO(q->color.g), NK_TO_CAIRO(q->color.b), NK_TO_CAIRO(q->color.a));
                cairo_set_line_width(cr, q->line_thickness);
                cairo_move_to(cr, q->begin.x, q->begin.y);
                cairo_curve_to(cr, q->ctrl[0].x, q->ctrl[0].y, q->ctrl[1].x, q->ctrl[1].y, q->end.x, q->end.y);
                cairo_stroke(cr);
            }
            break;
        case NK_COMMAND_RECT:
            {
                const struct nk_command_rect *r = (const struct nk_command_rect *)cmd;
                cairo_set_source_rgba(cr, NK_TO_CAIRO(r->color.r), NK_TO_CAIRO(r->color.g), NK_TO_CAIRO(r->color.b), NK_TO_CAIRO(r->color.a));
                cairo_set_line_width(cr, r->line_thickness);
                if (r->rounding == 0) {
                    cairo_rectangle(cr, r->x, r->y, r->w, r->h);
                }
                else {
                    int xl = r->x + r->w - r->rounding;
                    int xr = r->x + r->rounding;
                    int yl = r->y + r->h - r->rounding;
                    int yr = r->y + r->rounding;
                    cairo_new_sub_path(cr);
                    cairo_arc(cr, xl, yr, r->rounding, NK_CAIRO_DEG_TO_RAD(-90), NK_CAIRO_DEG_TO_RAD(0));
                    cairo_arc(cr, xl, yl, r->rounding, NK_CAIRO_DEG_TO_RAD(0), NK_CAIRO_DEG_TO_RAD(90));
                    cairo_arc(cr, xr, yl, r->rounding, NK_CAIRO_DEG_TO_RAD(90), NK_CAIRO_DEG_TO_RAD(180));
                    cairo_arc(cr, xr, yr, r->rounding, NK_CAIRO_DEG_TO_RAD(180), NK_CAIRO_DEG_TO_RAD(270));
                    cairo_close_path(cr);
                }
                cairo_stroke(cr);
            }
            break;
        case NK_COMMAND_RECT_FILLED:
            {
                const struct nk_command_rect_filled *r = (const struct nk_command_rect_filled *)cmd;
                cairo_set_source_rgba(cr, NK_TO_CAIRO(r->color.r), NK_TO_CAIRO(r->color.g), NK_TO_CAIRO(r->color.b), NK_TO_CAIRO(r->color.a));
                if (r->rounding == 0) {
                    cairo_rectangle(cr, r->x, r->y, r->w, r->h);
                } else {
                    int xl = r->x + r->w - r->rounding;
                    int xr = r->x + r->rounding;
                    int yl = r->y + r->h - r->rounding;
                    int yr = r->y + r->rounding;
                    cairo_new_sub_path(cr);
                    cairo_arc(cr, xl, yr, r->rounding, NK_CAIRO_DEG_TO_RAD(-90), NK_CAIRO_DEG_TO_RAD(0));
                    cairo_arc(cr, xl, yl, r->rounding, NK_CAIRO_DEG_TO_RAD(0), NK_CAIRO_DEG_TO_RAD(90));
                    cairo_arc(cr, xr, yl, r->rounding, NK_CAIRO_DEG_TO_RAD(90), NK_CAIRO_DEG_TO_RAD(180));
                    cairo_arc(cr, xr, yr, r->rounding, NK_CAIRO_DEG_TO_RAD(180), NK_CAIRO_DEG_TO_RAD(270));
                    cairo_close_path(cr);
                }
                cairo_fill(cr);
            }
            break;
        case NK_COMMAND_RECT_MULTI_COLOR:
            {
                /* from https://github.com/taiwins/twidgets/blob/master/src/nk_wl_cairo.c */
                const struct nk_command_rect_multi_color *r = (const struct nk_command_rect_multi_color *)cmd;
                cairo_pattern_t *pat = cairo_pattern_create_mesh();
                if (pat) {
                    cairo_mesh_pattern_begin_patch(pat);
                    cairo_mesh_pattern_move_to(pat, r->x, r->y);
                    cairo_mesh_pattern_line_to(pat, r->x, r->y + r->h);
                    cairo_mesh_pattern_line_to(pat, r->x + r->w, r->y + r->h);
                    cairo_mesh_pattern_line_to(pat, r->x + r->w, r->y);
                    cairo_mesh_pattern_set_corner_color_rgba(pat, 0, NK_TO_CAIRO(r->left.r), NK_TO_CAIRO(r->left.g), NK_TO_CAIRO(r->left.b), NK_TO_CAIRO(r->left.a));
                    cairo_mesh_pattern_set_corner_color_rgba(pat, 1, NK_TO_CAIRO(r->bottom.r), NK_TO_CAIRO(r->bottom.g), NK_TO_CAIRO(r->bottom.b), NK_TO_CAIRO(r->bottom.a));
                    cairo_mesh_pattern_set_corner_color_rgba(pat, 2, NK_TO_CAIRO(r->right.r), NK_TO_CAIRO(r->right.g), NK_TO_CAIRO(r->right.b), NK_TO_CAIRO(r->right.a));
                    cairo_mesh_pattern_set_corner_color_rgba(pat, 3, NK_TO_CAIRO(r->top.r), NK_TO_CAIRO(r->top.g), NK_TO_CAIRO(r->top.b), NK_TO_CAIRO(r->top.a));
                    cairo_mesh_pattern_end_patch(pat);

                    cairo_rectangle(cr, r->x, r->y, r->w, r->h);
                    cairo_set_source(cr, pat);
                    cairo_fill(cr);
                    cairo_pattern_destroy(pat);
                }
            }
            break;
        case NK_COMMAND_CIRCLE:
            {
                const struct nk_command_circle *c = (const struct nk_command_circle *)cmd;
                cairo_set_source_rgba(cr, NK_TO_CAIRO(c->color.r), NK_TO_CAIRO(c->color.g), NK_TO_CAIRO(c->color.b), NK_TO_CAIRO(c->color.a));
                cairo_set_line_width(cr, c->line_thickness);
                cairo_save(cr);
                cairo_translate(cr, c->x + c->w / 2.0, c->y + c->h / 2.0);
                cairo_scale(cr, c->w / 2.0, c->h / 2.0);
                cairo_arc(cr, 0, 0, 1, NK_CAIRO_DEG_TO_RAD(0), NK_CAIRO_DEG_TO_RAD(360));
                cairo_restore(cr);
                cairo_stroke(cr);
            }
            break;
        case NK_COMMAND_CIRCLE_FILLED:
            {
                const struct nk_command_circle_filled *c = (const struct nk_command_circle_filled *)cmd;
                cairo_set_source_rgba(cr, NK_TO_CAIRO(c->color.r), NK_TO_CAIRO(c->color.g), NK_TO_CAIRO(c->color.b), NK_TO_CAIRO(c->color.a));
                cairo_save(cr);
                cairo_translate(cr, c->x + c->w / 2.0, c->y + c->h / 2.0);
                cairo_scale(cr, c->w / 2.0, c->h / 2.0);
                cairo_arc(cr, 0, 0, 1, NK_CAIRO_DEG_TO_RAD(0), NK_CAIRO_DEG_TO_RAD(360));
                cairo_restore(cr);
                cairo_fill(cr);
            }
            break;
        case NK_COMMAND_ARC:
            {
                const struct nk_command_arc *a = (const struct nk_command_arc*) cmd;
                cairo_set_source_rgba(cr, NK_TO_CAIRO(a->color.r), NK_TO_CAIRO(a->color.g), NK_TO_CAIRO(a->color.b), NK_TO_CAIRO(a->color.a));
                cairo_set_line_width(cr, a->line_thickness);
                cairo_arc(cr, a->cx, a->cy, a->r, NK_CAIRO_DEG_TO_RAD(a->a[0]), NK_CAIRO_DEG_TO_RAD(a->a[1]));
                cairo_stroke(cr);
            }
            break;
        case NK_COMMAND_ARC_FILLED:
            {
                const struct nk_command_arc_filled *a = (const struct nk_command_arc_filled*)cmd;
                cairo_set_source_rgba(cr, NK_TO_CAIRO(a->color.r), NK_TO_CAIRO(a->color.g), NK_TO_CAIRO(a->color.b), NK_TO_CAIRO(a->color.a));
                cairo_arc(cr, a->cx, a->cy, a->r, NK_CAIRO_DEG_TO_RAD(a->a[0]), NK_CAIRO_DEG_TO_RAD(a->a[1]));
                cairo_fill(cr);
            }
            break;
        case NK_COMMAND_TRIANGLE:
            {
                const struct nk_command_triangle *t = (const struct nk_command_triangle *)cmd;
                cairo_set_source_rgba(cr, NK_TO_CAIRO(t->color.r), NK_TO_CAIRO(t->color.g), NK_TO_CAIRO(t->color.b), NK_TO_CAIRO(t->color.a));
                cairo_set_line_width(cr, t->line_thickness);
                cairo_move_to(cr, t->a.x, t->a.y);
                cairo_line_to(cr, t->b.x, t->b.y);
                cairo_line_to(cr, t->c.x, t->c.y);
                cairo_close_path(cr);
                cairo_stroke(cr);
            }
            break;
        case NK_COMMAND_TRIANGLE_FILLED:
            {
                const struct nk_command_triangle_filled *t = (const struct nk_command_triangle_filled *)cmd;
                cairo_set_source_rgba(cr, NK_TO_CAIRO(t->color.r), NK_TO_CAIRO(t->color.g), NK_TO_CAIRO(t->color.b), NK_TO_CAIRO(t->color.a));
                cairo_move_to(cr, t->a.x, t->a.y);
                cairo_line_to(cr, t->b.x, t->b.y);
                cairo_line_to(cr, t->c.x, t->c.y);
                cairo_close_path(cr);
                cairo_fill(cr);
            }
            break;
        case NK_COMMAND_POLYGON:
            {
                int i;
                const struct nk_command_polygon *p = (const struct nk_command_polygon *)cmd;
                cairo_set_source_rgba(cr, NK_TO_CAIRO(p->color.r), NK_TO_CAIRO(p->color.g), NK_TO_CAIRO(p->color.b), NK_TO_CAIRO(p->color.a));
                cairo_set_line_width(cr, p->line_thickness);
                cairo_move_to(cr, p->points[0].x, p->points[0].y);
                for (i = 1; i < p->point_count; ++i) {
                    cairo_line_to(cr, p->points[i].x, p->points[i].y);
                }
                cairo_close_path(cr);
                cairo_stroke(cr);
            }
            break;
        case NK_COMMAND_POLYGON_FILLED:
            {
                int i;
                const struct nk_command_polygon_filled *p = (const struct nk_command_polygon_filled *)cmd;
                cairo_set_source_rgba (cr, NK_TO_CAIRO(p->color.r), NK_TO_CAIRO(p->color.g), NK_TO_CAIRO(p->color.b), NK_TO_CAIRO(p->color.a));
                cairo_move_to(cr, p->points[0].x, p->points[0].y);
                for (i = 1; i < p->point_count; ++i) {
                    cairo_line_to(cr, p->points[i].x, p->points[i].y);
                }
                cairo_close_path(cr);
                cairo_fill(cr);
            }
            break;
        case NK_COMMAND_POLYLINE:
            {
                int i;
                const struct nk_command_polyline *p = (const struct nk_command_polyline *)cmd;
                cairo_set_source_rgba(cr, NK_TO_CAIRO(p->color.r), NK_TO_CAIRO(p->color.g), NK_TO_CAIRO(p->color.b), NK_TO_CAIRO(p->color.a));
                cairo_set_line_width(cr, p->line_thickness);
                cairo_move_to(cr, p->points[0].x, p->points[0].y);
                for (i = 1; i < p->point_count; ++i) {
                    cairo_line_to(cr, p->points[i].x, p->points[i].y);
                }
                cairo_stroke(cr);
            }
            break;
            case NK_COMMAND_TEXT: {
                const struct nk_command_text *t = (const struct nk_command_text *)cmd;
                const struct nk_cairo_font *font = (struct nk_cairo_font *)t->font->userdata.ptr;
                cairo_set_source_rgba(cr, NK_TO_CAIRO(t->foreground.r), NK_TO_CAIRO(t->foreground.g), NK_TO_CAIRO(t->foreground.b), NK_TO_CAIRO(t->foreground.a));
                
                // Position
                cairo_move_to(cr, t->x, t->y);

                // Build Pango layout
                PangoLayout *layout = pango_cairo_create_layout(cr);
                pango_layout_set_text(layout, t->string, t->length);
                pango_layout_set_font_description(layout,  font->desc);
                pango_cairo_show_layout(cr, layout);
                g_object_unref(layout);
                cairo_restore(cr);
                break;
            }
            break;
        case NK_COMMAND_IMAGE:
            {
                /* from https://github.com/taiwins/twidgets/blob/master/src/nk_wl_cairo.c */
                const struct nk_command_image *im = (const struct nk_command_image *)cmd;
                cairo_surface_t *img_surf;
                double sw = (double)im->w / (double)im->img.region[2];
                double sh = (double)im->h / (double)im->img.region[3];
                cairo_format_t format = CAIRO_FORMAT_ARGB32;
                int stride = cairo_format_stride_for_width(format, im->img.w);

                if (!im->img.handle.ptr) return nk_false;
                img_surf = cairo_image_surface_create_for_data((unsigned char *)im->img.handle.ptr, format, im->img.w, im->img.h, stride);
                if (!img_surf) return nk_false;
                cairo_save(cr);

                cairo_rectangle(cr, im->x, im->y, im->w, im->h);
                /* scale here, if after source set, the scale would not apply to source
                 * surface
                 */
                cairo_scale(cr, sw, sh);
                /* the coordinates system in cairo is not intuitive, scale, translate,
                 * are applied to source. Refer to
                 * "https://www.cairographics.org/FAQ/#paint_from_a_surface" for details
                 *
                 * if you set source_origin to (0,0), it would be like source origin
                 * aligned to dest origin, then if you draw a rectangle on (x, y, w, h).
                 * it would clip out the (x, y, w, h) of the source on you dest as well.
                 */
                cairo_set_source_surface(cr, img_surf, im->x/sw - im->img.region[0], im->y/sh - im->img.region[1]);
                cairo_fill(cr);

                cairo_restore(cr);
                cairo_surface_destroy(img_surf);
            }
            break;
        case NK_COMMAND_CUSTOM:
            {
	            const struct nk_command_custom *cu = (const struct nk_command_custom *)cmd;
                if (cu->callback) {
                    cu->callback(cr, cu->x, cu->y, cu->w, cu->h, cu->callback_data);
                }
            }
        default:
            break;
        }
    }

    nk_clear(nk_ctx);
    cairo_pop_group_to_source(cr);
    cairo_paint(cr);
    cairo_surface_flush(cairo_ctx->surface);

    EXT();
    return nk_true;
}

NK_API void nk_cairo_dump_surface(struct nk_cairo_context *cairo_ctx, const char *filename)
{
    ENT();
    if (cairo_ctx == NULL)  {
        ERR("Invalid parameter");
        return;
    }
    DBG("Writing surface %p to file %s", cairo_ctx->surface, filename);
    cairo_surface_write_to_png(cairo_ctx->surface, filename);
    EXT();
}

#endif /* NK_CAIRO_IMPLEMENTATION */
