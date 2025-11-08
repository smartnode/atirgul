#include <iostream>
#include <string>
#include <vector>
#include <cstdint>

// --- FreeType for font loading ---
#include <ft2build.h>
#include FT_FREETYPE_H

// --- HarfBuzz for text shaping ---
#include <hb.h>
#include <hb-ft.h>

// --- FriBidi for bidirectional text reordering (Arabic/Hebrew) ---
#include <fribidi.h>

// --- Cairo for drawing output ---
#include <cairo.h>
#include <cairo-ft.h>

int main() {
    // Path to a Unicode-compatible font (Arial Unicode covers many scripts)
    const char* fontpath = "/System/Library/Fonts/Supplemental/Arial Unicode.ttf";

    // Input text containing English, Hebrew, Arabic, and Hindi
    std::string text = u8"English עברית العربية हिन्दी";

    // -------------------------------
    // 1️⃣ Initialize FreeType library
    // -------------------------------
    FT_Library ft;
    if (FT_Init_FreeType(&ft)) {
        std::cerr << "Error: Could not initialize FreeType.\n";
        return 1;
    }

    // Load the font face from file
    FT_Face face;
    if (FT_New_Face(ft, fontpath, 0, &face)) {
        std::cerr << "Error: Could not load font: " << fontpath << "\n";
        return 1;
    }

    // Set character size in 1/64th of points (48 pt)
    FT_Set_Char_Size(face, 0, 48 * 64, 0, 0);

    // ----------------------------------------
    // 2️⃣ Convert UTF-8 → Unicode (FriBidi input)
    // ----------------------------------------
    std::vector<FriBidiChar> logical(text.size()); // allocate buffer
    FriBidiStrIndex length = fribidi_charset_to_unicode(
        FRIBIDI_CHAR_SET_UTF8, // source charset
        text.c_str(),          // UTF-8 input string
        text.size(),           // input length in bytes
        logical.data()         // output Unicode buffer
    );
    logical.resize(length); // trim buffer to actual used length

    // -------------------------------------------------------
    // 3️⃣ Use FriBidi to reorder logical → visual (bidi text)
    // -------------------------------------------------------
    std::vector<FriBidiChar> visual(length); // output buffer for reordered text
    FriBidiParType base_dir = FRIBIDI_PAR_ON; // let FriBidi auto-detect paragraph direction

    // Perform the logical-to-visual transformation
    fribidi_log2vis(
        logical.data(),  // input Unicode array
        length,          // number of characters
        &base_dir,       // base paragraph direction
        visual.data(),   // output (visual) Unicode array
        nullptr, nullptr, nullptr // optional maps we don't need
    );

    // ----------------------------------------------------------
    // 4️⃣ Convert reordered Unicode → UTF-8 string for HarfBuzz
    // ----------------------------------------------------------
    std::vector<char> visual_utf8(length * 4 + 1); // allocate enough space for UTF-8
    fribidi_unicode_to_charset(
        FRIBIDI_CHAR_SET_UTF8, // target charset
        visual.data(),         // input Unicode
        length,                // number of chars
        visual_utf8.data()     // output UTF-8 bytes
    );
    std::string visual_text(visual_utf8.data()); // create std::string

    // -------------------------------------------------
    // 5️⃣ Shape text using HarfBuzz (script-aware layout)
    // -------------------------------------------------
    // Create HarfBuzz font from FreeType face
    hb_font_t* hb_font = hb_ft_font_create(face, nullptr);

    // Create a new buffer to hold text for shaping
    hb_buffer_t* buf = hb_buffer_create();

    // Add UTF-8 text to the buffer
    hb_buffer_add_utf8(buf, visual_text.c_str(), -1, 0, -1);

    // Let HarfBuzz detect script, direction, and language
    hb_buffer_guess_segment_properties(buf);

    // Shape the text: convert characters → glyphs + positions
    hb_shape(hb_font, buf, nullptr, 0);

    // Retrieve shaped glyph information
    unsigned int glyph_count;
    hb_glyph_info_t* info = hb_buffer_get_glyph_infos(buf, &glyph_count);
    hb_glyph_position_t* pos = hb_buffer_get_glyph_positions(buf, &glyph_count);

    // ----------------------------------------
    // 6️⃣ Create Cairo surface and draw output
    // ----------------------------------------
    // Create an ARGB32 image surface (white background)
    cairo_surface_t* surface =
        cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 1200, 200);
    cairo_t* cr = cairo_create(surface);

    // Fill the background with white
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_paint(cr);

    // Set text color to black
    cairo_set_source_rgb(cr, 0, 0, 0);

    // Link Cairo to our FreeType font
    cairo_font_face_t* cr_face = cairo_ft_font_face_create_for_ft_face(face, 0);
    cairo_set_font_face(cr, cr_face);
    cairo_set_font_size(cr, 48);

    // Starting draw position
    double x = 50, y = 100;

    // Iterate through shaped glyphs and draw them
    for (unsigned int i = 0; i < glyph_count; ++i) {
        cairo_glyph_t glyph;
        glyph.index = info[i].codepoint;            // glyph index in font
        glyph.x = x + pos[i].x_offset / 64.0;       // apply subpixel X offset
        glyph.y = y - pos[i].y_offset / 64.0;       // apply subpixel Y offset

        // Draw one glyph
        cairo_show_glyphs(cr, &glyph, 1);

        // Advance pen position for next glyph
        x += pos[i].x_advance / 64.0;
        y -= pos[i].y_advance / 64.0;
    }

    // Save result as PNG file
    cairo_surface_write_to_png(surface, "output.png");
    std::cout << "✅ Rendered multilingual text to output.png\n";

    // ----------------------------------------
    // 7️⃣ Cleanup (always free native resources)
    // ----------------------------------------
    cairo_destroy(cr);
    cairo_surface_destroy(surface);
    cairo_font_face_destroy(cr_face);
    hb_buffer_destroy(buf);
    hb_font_destroy(hb_font);
    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    return 0;
}
