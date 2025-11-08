#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <cmath>
#include <algorithm>

#include "nuklear.h"
#include "nuklear_cairo.h"

// --- Configuration ---
constexpr float LANGUAGE_DURATION_SECONDS = 5.0f;
constexpr int MAX_PROGRESS = 100;

// Structure to hold multilingual data
struct LanguageData
{
    std::string title;
    std::string description;
    std::string progress_text;
};

const std::vector<LanguageData> LANGUAGES = {
    {"Beautiful Rose", "The rose is a beautiful flower with numerous varieties and colors. It symbolizes love, beauty, and passion.", "English (en)"},
    {"아름다운 장미", "장미는 수많은 품종과 색상을 가진 아름다운 꽃입니다. 사랑, 아름다움, 열정을 상징합니다.", "한국어 (ko)"},
    {"ורד יפהפה", "הוורד הוא פרח יפהפה עם זנים וצבעים רבים. הוא מסמל אהבה, יופי ותשוקה.", "עברית (he)"},
    {"وردة جميلة", "الوردة هي زهرة جميلة ذات أصناف وألوان عديدة. إنها ترمز إلى الحب والجمال والعاطفة.", "العربية (ar)"},
    {"सुंदर गुलाब", "गुलाब एक सुंदर फूल है जिसके कई प्रकार और रंग हैं। यह प्रेम, सौंदर्य और जुनून का प्रतीक है।", "हिन्दी (hi)"},
    {"Go'zal atirgul", "Atirgul - ko'p navlari va ranglari bo'lgan go'zal gul. U sevgi, go'zallik va ehtirosni anglatadi.", "O'zbek (uz)"}};

// --- State Management ---
struct AppState
{
    bool running;
    int language_index = 0;
    float current_progress_percent = 0.0f;
    std::chrono::steady_clock::time_point last_switch_time;
    int width;
    int height;
    int draw_width;
    int draw_height;
    int bpp;
};

// --- Helper Functions ---
void update_state(AppState &state)
{
    using namespace std::chrono;

    auto now = steady_clock::now();
    duration<float> elapsed_duration = now - state.last_switch_time;
    float elapsed_seconds = elapsed_duration.count();

    if (elapsed_seconds >= LANGUAGE_DURATION_SECONDS)
    {
        state.language_index = (state.language_index + 1) % LANGUAGES.size();
        state.last_switch_time = now;
        elapsed_seconds = 0.0f;
    }

    float language_segment_width = (float)MAX_PROGRESS / LANGUAGES.size();
    float base_progress = state.language_index * language_segment_width;
    float interval_progress = (elapsed_seconds / LANGUAGE_DURATION_SECONDS) * language_segment_width;

    state.current_progress_percent = base_progress + interval_progress;

    if (state.current_progress_percent > MAX_PROGRESS)
    {
        state.current_progress_percent = MAX_PROGRESS;
    }
}

void draw_multilingual_window(struct nk_context *ctx, AppState &state)
{
    const LanguageData &lang = LANGUAGES[state.language_index];

    int wmargin = (state.width > state.draw_width) ? (state.width - state.draw_width) / 2 : 0;
    int hmargin = (state.height > state.draw_height) ? (state.height - state.draw_height) / 2 : 0;

    std::string progress_percent_str = std::to_string(std::round(state.current_progress_percent * 10) / 10.0f) + " %";

    nk_size temp_progress = (nk_size)std::round(state.current_progress_percent);

    if (nk_begin(ctx, lang.title.c_str(), nk_rect(wmargin, hmargin, state.draw_width, state.draw_height), NK_WINDOW_BORDER | NK_WINDOW_NO_SCROLLBAR))
    {
        // Title Row
        nk_layout_row_dynamic(ctx, 30, 1);
        nk_label(ctx, lang.title.c_str(), NK_TEXT_CENTERED);

        // Description Row
        nk_layout_row_dynamic(ctx, 50, 1);
        nk_label_wrap(ctx, lang.description.c_str());

        // Spacer Row
        nk_layout_row_dynamic(ctx, 10, 1);
        nk_spacer(ctx);

        // Progress Bar Row (5% spacer | 85% progress bar | 10% progress text)
        nk_layout_row_begin(ctx, NK_DYNAMIC, 25, 3);

        // 5% Spacer
        nk_layout_row_push(ctx, 0.05f);
        nk_spacer(ctx);

        // 85% Progress Bar
        nk_layout_row_push(ctx, 0.85f);
        nk_progress(ctx, &temp_progress, MAX_PROGRESS, NK_FIXED);

        // 10% Progress Text
        nk_layout_row_push(ctx, 0.10f);
        nk_label(ctx, progress_percent_str.c_str(), NK_TEXT_CENTERED);

        nk_layout_row_end(ctx);

        // Spacer Row
        nk_layout_row_dynamic(ctx, 10, 1);
        nk_spacer(ctx);

        // Language Name Row
        nk_layout_row_dynamic(ctx, 20, 1);
        nk_label_colored(ctx, lang.progress_text.c_str(), NK_TEXT_RIGHT, nk_rgb(255, 150, 200));
    }
    nk_end(ctx);
}

int main()
{
    AppState state;
    state.last_switch_time = std::chrono::steady_clock::now();
    state.running = true;

    state.width = 700;
    state.height = 580;
    state.draw_width = 600;
    state.draw_height = 480;
    state.bpp = 4;

    size_t buffer_size = state.width * state.height * state.bpp;
    auto buffer = std::vector<uint8_t>(buffer_size);

    auto cairo_ctx = nk_cairo_init(buffer.data(), state.width, state.height, state.bpp, NK_CARIO_ROTATE_0);
    if (!cairo_ctx)
    {
        std::cerr << "Failed to init nk cairo" << std::endl;
        return -1;
    }

    auto nk_ctx = nk_cairo_get_nk_context(cairo_ctx);
    if (!nk_ctx)
    {
        std::cerr << "Failed to get nk context" << std::endl;
        return -1;
    }

    // while (state.running)
    {
        update_state(state);
        draw_multilingual_window(nk_ctx, state);
        nk_cairo_damage(cairo_ctx);
        if (nk_cairo_render(cairo_ctx) == false)
        {
            ERR("Cairo render faileds");
        }
        nk_cairo_dump_surface(cairo_ctx, "dump.png");
    }

    nk_cairo_deinit(cairo_ctx);

    return 0;
}