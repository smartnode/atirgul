#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <cmath>
#include <algorithm>
#include "nuklear.h"
#include "nuklear_cairo.h"

// --- Configuration ---
constexpr int MAX_PROGRESS = 100;

// Structure to hold multilingual data
struct LanguageData
{
    std::string code;
    std::string title;
    std::string description;
    std::string name;
};

// Define LANGUAGES vector as before
const std::vector<LanguageData> LANGUAGES = {
    {"en", "Beautiful Rose", "The rose is a beautiful flower with numerous varieties and colors. It symbolizes love, beauty, and passion.", "English"},
    {"ko", "아름다운 장미", "장미는 수많은 품종과 색상을 가진 아름다운 꽃입니다. 사랑, 아름다움, 열정을 상징합니다.", "한국어"},
    {"he", "ורד יפהפה", "הוורד הוא פרח יפהפה עם זנים וצבעים רבים. הוא מסמל אהבה, יופי ותשוקה.", "עברית"},
    {"ar", "وردة جميلة", "الوردة هي زهرة جميلة ذات أصناف وألوان عديدة. إنها ترمز إلى الحب والجمال والعاطفة.", "العربية"},
    {"hi", "सुंदर गुलाब", "गुलाब एक सुंदर फूल है जिसके कई प्रकार और रंग हैं। यह प्रेम, सौंदर्य और जुनून का प्रतीक है।", "हिन्दी "},
};

static float PROGRESS_STEP = static_cast<float>(MAX_PROGRESS) / LANGUAGES.size();

// --- State Management ---
struct AppData
{
    struct nk_cairo_context *cairo_ctx;
    struct nk_user_font *title_font;
    struct nk_user_font *desc_font;
    bool running;
    int language_index = 0;
    int current_progress = 0;
    int width;
    int height;
    int draw_width;
    int draw_height;
    int bpp;
};

/**
 * @brief Draws the multilingual window and handles button logic.
 */
void draw_multilingual_window(struct nk_context *ctx, AppData &data)
{
    const LanguageData &lang = LANGUAGES[data.language_index];
    int wmargin = (data.width > data.draw_width) ? (data.width - data.draw_width) / 2 : 0;
    int hmargin = (data.height > data.draw_height) ? (data.height - data.draw_height) / 2 : 0;

    std::string progress_str = std::to_string(data.current_progress) + " %";
    auto progress_nk = static_cast<nk_size>(data.current_progress);

    auto window_flags = NK_WINDOW_BORDER | NK_WINDOW_NO_SCROLLBAR;
    if (nk_begin(ctx, lang.title.c_str(), nk_rect(wmargin, hmargin, data.draw_width, data.draw_height), window_flags))
    {
        // 1. Title Row
        nk_layout_row_dynamic(ctx, 50, 1);
        nk_style_set_font(ctx, data.title_font);
        nk_label(ctx, lang.title.c_str(), NK_TEXT_CENTERED);

        // 2. Description Row
        nk_layout_row_begin(ctx, NK_DYNAMIC, 70, 2);
        nk_layout_row_push(ctx, 0.10f);
        nk_spacer(ctx);
        nk_layout_row_push(ctx, 0.80f);
        nk_style_set_font(ctx, data.desc_font);
        nk_label_wrap(ctx, lang.description.c_str());
        nk_layout_row_end(ctx);

        // 3. Spacer
        nk_layout_row_dynamic(ctx, 10, 1);
        nk_spacer(ctx);

        // 4. Progress Bar Row (5% spacer | 85% progress bar | 10% progress text)
        nk_layout_row_begin(ctx, NK_DYNAMIC, 25, 3);

        // 5% Spacer
        nk_layout_row_push(ctx, 0.05f);
        nk_spacer(ctx);

        // 85% Progress Bar
        nk_layout_row_push(ctx, 0.85f);
        nk_progress(ctx, &progress_nk, MAX_PROGRESS, NK_FIXED);

        // 10% Progress Text
        nk_layout_row_push(ctx, 0.10f);
        nk_label(ctx, progress_str.c_str(), NK_TEXT_CENTERED);
        nk_layout_row_end(ctx);

        // 4. Spacer
        nk_layout_row_dynamic(ctx, 10, 1);
        nk_spacer(ctx);

        // 5. Language Name Row
        nk_layout_row_dynamic(ctx, 20, 1);
        nk_label(ctx, lang.name.c_str(), NK_TEXT_CENTERED);

        // 6. Button Spacer
        nk_layout_row_dynamic(ctx, 20, 1);
        nk_spacer(ctx);

        // 7. Prev/Next Buttons Row (1/3 spacer | 1/6 button | 1/6 spacer | 1/6 button | 1/3 spacer)
        nk_layout_row_begin(ctx, NK_DYNAMIC, 30, 5);

        // 1/3 Spacer (33%)
        nk_layout_row_push(ctx, 0.33f);
        nk_spacer(ctx);

        // Prev Button (16%)
        nk_layout_row_push(ctx, 0.16f);
        if (nk_button_label(ctx, "Prev"))
        {
            // Decrement index and wrap around
            data.language_index = (data.language_index - 1 + LANGUAGES.size()) % LANGUAGES.size();
            data.current_progress = (data.language_index + 1) * PROGRESS_STEP;
            nk_cairo_damage(data.cairo_ctx);
        }

        // Space between buttons (2%)
        nk_layout_row_push(ctx, 0.02f);
        nk_spacer(ctx);

        // Next Button (16%)
        nk_layout_row_push(ctx, 0.16f);
        if (nk_button_label(ctx, "Next"))
        {
            // Increment index and wrap around
            data.language_index = (data.language_index + 1) % LANGUAGES.size();
            data.current_progress = (data.language_index + 1) * PROGRESS_STEP;
            nk_cairo_damage(data.cairo_ctx);
        }

        // 1/3 Spacer (33%)
        nk_layout_row_push(ctx, 0.33f);
        nk_spacer(ctx);

        nk_layout_row_end(ctx);
    }
    nk_end(ctx);
}

int main()
{
    AppData data;
    data.running = true;
    data.width = 800;
    data.height = 600;
    data.draw_width = 600;
    data.draw_height = 480;
    data.bpp = 4;
    data.language_index = 0;
    data.current_progress = (data.language_index + 1) * PROGRESS_STEP;

    size_t buffer_size = data.width * data.height * data.bpp;
    auto buffer = std::vector<uint8_t>(buffer_size);

    data.cairo_ctx = nk_cairo_init(buffer.data(), data.width, data.height, data.bpp, NK_CARIO_ROTATE_0);
    if (!data.cairo_ctx)
    {
        std::cerr << "Failed to init nk cairo" << std::endl;
        return -1;
    }
    auto nk_ctx = nk_cairo_get_nk_context(data.cairo_ctx);
    if (!nk_ctx)
    {
        std::cerr << "Failed to get nk context" << std::endl;
        nk_cairo_deinit(data.cairo_ctx);
        return -1;
    }

    data.title_font = nk_cairo_get_font(data.cairo_ctx, "Arial", 20.0f);
    data.desc_font = nk_cairo_get_font(data.cairo_ctx, "Arial", 14.0f);

    //while (data.running)
    for (data.language_index = 0; data.language_index < LANGUAGES.size(); data.language_index++)
    {
        data.current_progress = (data.language_index + 1) * PROGRESS_STEP;

        // Pass cairo_ctx to draw function so buttons can trigger damage
        draw_multilingual_window(nk_ctx, data);

        if (nk_cairo_render(data.cairo_ctx) == false)
        {
            std::cerr << "Cairo render failed" << std::endl;
            break;
        }

        std::string dump_file = std::to_string(data.language_index + 1) + "_dump.png";
        nk_cairo_dump_surface(data.cairo_ctx, dump_file.c_str());
    }

    nk_cairo_deinit(data.cairo_ctx);
    return 0;
}