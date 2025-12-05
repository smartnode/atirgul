#import <Cocoa/Cocoa.h>
// Objective-C++ Cocoa example for atirgul nuklear_cairo backend
#include <objc/objc.h>
#import <objc/runtime.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <cmath>
#include "nuklear.h"
#include "nuklear_cairo.h"

// Reuse the UI layout from the C++ example
struct LanguageData
{
    std::string code;
    std::string title;
    std::string description;
    std::string name;
};

static const std::vector<LanguageData> LANGUAGES = {
    {"en", "Beautiful Rose", "The rose is a beautiful flower with numerous varieties and colors. It symbolizes love, beauty, and passion.", "English"},
    {"ko", "아름다운 장미", "장미는 수많은 품종과 색상을 가진 아름다운 꽃입니다. 사랑, 아름다움, 열정을 상징합니다.", "한국어"},
    {"he", "ורד יפהפה", "הוורד הוא פרח יפהפה עם זנים וצבעים רבים. הוא מסמל אהבה, יופי ותשוקה.", "עברית"},
    {"ar", "وردة جميلة", "الوردة هي زهرة جميلة ذات أصناف وألوان عديدة. إنها ترمز إلى الحب والجمال والعاطفة.", "العربية"},
    {"hi", "सुंदर गुलाब", "गुलाब एक सुंदर फूल है जिसके कई प्रकार और रंग हैं। यह प्रेम, सौंदर्य और जुनून का प्रतीक है।", "हिन्दी "},
};

static const int MAX_PROGRESS = 100;

struct AppData {
    struct nk_cairo_context *cairo_ctx;
    struct nk_user_font *title_font;
    struct nk_user_font *desc_font;
    bool running;
    int language_index;
    int current_progress;
    int width;
    int height;
    int draw_width;
    int draw_height;
    int bpp;
    uint8_t *buffer;
    size_t buffer_size;
};

static float PROGRESS_STEP = static_cast<float>(MAX_PROGRESS) / LANGUAGES.size();

// Forward declaration
static void draw_multilingual_window(struct nk_context *ctx, AppData &data);

// Simple NSView subclass that will present the buffer
@interface CairoView : NSView {
    AppData *appData;
    CGImageRef cgImage;
}
- (instancetype)initWithFrame:(NSRect)frame appData:(AppData *)data;
- (void)updateImage;
- (void)cleanupImage;
@end

@implementation CairoView
- (instancetype)initWithFrame:(NSRect)frame appData:(AppData *)data
{
    self = [super initWithFrame:frame];
    if (self) {
        appData = data;
        cgImage = NULL;
        [self setWantsLayer:YES];
    }
    return self;
}

#if !__has_feature(objc_arc)
- (void)dealloc
{
    if (cgImage) CGImageRelease(cgImage);
    [super dealloc];
}
#else
- (void)dealloc
{
    if (cgImage) CGImageRelease(cgImage);
}
#endif

- (void)cleanupImage {
    if (cgImage) { CGImageRelease(cgImage); cgImage = NULL; }
}

- (void)updateImage
{
    // Create CGImage from raw buffer. Assumes CAIRO_FORMAT_ARGB32 premultiplied.
    if (!appData || !appData->buffer) return;

    if (cgImage) { CGImageRelease(cgImage); cgImage = NULL; }

    int w = appData->width;
    int h = appData->height;
    size_t stride = w * 4;

    CGDataProviderRef provider = CGDataProviderCreateWithData(NULL, appData->buffer, appData->buffer_size, NULL);
    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();

    CGBitmapInfo info = (CGBitmapInfo)(kCGBitmapByteOrder32Little | (uint32_t)kCGImageAlphaPremultipliedFirst);

    cgImage = CGImageCreate(w, h, 8, 32, stride, colorSpace, info, provider, NULL, false, kCGRenderingIntentDefault);

    CGColorSpaceRelease(colorSpace);
    CGDataProviderRelease(provider);
}

- (void)drawRect:(NSRect)dirtyRect
{
    [super drawRect:dirtyRect];

    if (!cgImage) return;

    NSGraphicsContext *nsctx = [NSGraphicsContext currentContext];
    CGContextRef ctx = nsctx.CGContext;

    // Draw the CGImage directly into the view's context. If the image appears
    // vertically flipped, remove this flip logic; currently we draw without
    // transforming the context so the image's top matches the view's top.
    CGRect dest = CGRectMake(0, 0, self.bounds.size.width, self.bounds.size.height);
    CGContextSaveGState(ctx);
    // On macOS AppKit the view coordinate system origin is at the bottom-left
    // for layer-backed views; however CGImage drawing expects the destination
    // rect in the current coordinate system. Drawing without an extra vertical
    // flip aligns Cairo's image top to the view top.
    CGContextDrawImage(ctx, dest, cgImage);
    CGContextRestoreGState(ctx);
}
@end

// Window delegate to handle close and resize events
@interface WindowDelegate : NSObject <NSWindowDelegate> {
    AppData *appData;
    NSTimer *timerRef;
    CairoView *cview;
}
- (instancetype)initWithAppData:(AppData *)data timer:(NSTimer*)timer view:(CairoView*)view;
@end

@implementation WindowDelegate
- (instancetype)initWithAppData:(AppData *)data timer:(NSTimer*)timer view:(CairoView*)view
{
    self = [super init];
    if (self) {
        appData = data;
        timerRef = timer;
        cview = view;
    }
    return self;
}

- (void)windowWillClose:(NSNotification *)notification
{
    // Stop the timer and terminate the app so main continues and we can cleanup
    if (timerRef) {
        [timerRef invalidate];
        timerRef = nil;
    }
    [NSApp terminate:nil];
    // As a fallback, ensure process exits
    exit(0);
}

- (void)windowDidResize:(NSNotification *)notification
{
    NSWindow *w = (NSWindow *)notification.object;
    NSRect contentBounds = w.contentView.bounds;
    double scale = 1.0;
    int newW = (int)round(contentBounds.size.width * scale);
    int newH = (int)round(contentBounds.size.height * scale);
    if (newW <= 0 || newH <= 0) return;
    if (appData->width == newW && appData->height == newH) return;

    // Recreate buffer and cairo context for new size
    if (appData->cairo_ctx) {
        nk_cairo_deinit(appData->cairo_ctx);
        appData->cairo_ctx = NULL;
    }
    if (appData->buffer) {
        free(appData->buffer);
        appData->buffer = NULL;
    }

    appData->width = newW;
    appData->height = newH;
    appData->buffer_size = (size_t)appData->width * (size_t)appData->height * (size_t)appData->bpp;
    appData->buffer = (uint8_t*)malloc(appData->buffer_size);
    if (!appData->buffer) {
        fprintf(stderr, "Failed to reallocate buffer for resize\n");
        return;
    }
    memset(appData->buffer, 0, appData->buffer_size);

    appData->cairo_ctx = nk_cairo_init(appData->buffer, appData->width, appData->height, appData->bpp, NK_CARIO_ROTATE_0);
    if (!appData->cairo_ctx) {
        fprintf(stderr, "Failed to reinit nk_cairo after resize\n");
        return;
    }
    appData->title_font = nk_cairo_get_font(appData->cairo_ctx, "Arial", 20.0f);
    appData->desc_font = nk_cairo_get_font(appData->cairo_ctx, "Arial", 14.0f);

    // Update draw dimensions to new pixel size
    appData->draw_width = appData->width;
    appData->draw_height = appData->height;

    // Force a repaint
    nk_cairo_damage(appData->cairo_ctx);
    [cview updateImage];
    [cview setNeedsDisplay:YES];
}
@end

// Minimal input mapping: we track mouse location and button state
static double last_mouse_x = 0;
static double last_mouse_y = 0;
static bool mouse_down = false;

static void draw_multilingual_window(struct nk_context *ctx, AppData &data)
{
    const LanguageData &lang = LANGUAGES[data.language_index];

    std::string progress_str = std::to_string(data.current_progress) + " %";
    auto progress_nk = static_cast<nk_size>(data.current_progress);

    auto window_flags = NK_WINDOW_BORDER | NK_WINDOW_NO_SCROLLBAR;
    if (nk_begin(ctx, lang.title.c_str(), nk_rect(0, 0, data.draw_width, data.draw_height), window_flags))
    {
        nk_layout_row_dynamic(ctx, 50, 1);
        nk_style_set_font(ctx, data.title_font);
        nk_label(ctx, lang.title.c_str(), NK_TEXT_CENTERED);

        nk_layout_row_begin(ctx, NK_DYNAMIC, 70, 2);
        nk_layout_row_push(ctx, 0.10f);
        nk_spacer(ctx);
        nk_layout_row_push(ctx, 0.80f);
        nk_style_set_font(ctx, data.desc_font);
        nk_label_wrap(ctx, lang.description.c_str());
        nk_layout_row_end(ctx);

        nk_layout_row_dynamic(ctx, 10, 1);
        nk_spacer(ctx);

        nk_layout_row_begin(ctx, NK_DYNAMIC, 25, 3);
        nk_layout_row_push(ctx, 0.05f);
        nk_spacer(ctx);
        nk_layout_row_push(ctx, 0.85f);
        nk_progress(ctx, &progress_nk, MAX_PROGRESS, NK_FIXED);
        nk_layout_row_push(ctx, 0.10f);
        nk_label(ctx, progress_str.c_str(), NK_TEXT_CENTERED);
        nk_layout_row_end(ctx);

        nk_layout_row_dynamic(ctx, 10, 1);
        nk_spacer(ctx);

        nk_layout_row_dynamic(ctx, 20, 1);
        nk_label(ctx, lang.name.c_str(), NK_TEXT_CENTERED);

        nk_layout_row_dynamic(ctx, 20, 1);
        nk_spacer(ctx);

        nk_layout_row_begin(ctx, NK_DYNAMIC, 30, 5);
        nk_layout_row_push(ctx, 0.33f);
        nk_spacer(ctx);
        nk_layout_row_push(ctx, 0.16f);
        if (nk_button_label(ctx, "Prev")) {
            data.language_index = (data.language_index - 1 + LANGUAGES.size()) % LANGUAGES.size();
            data.current_progress = (data.language_index + 1) * PROGRESS_STEP;
            nk_cairo_damage(data.cairo_ctx);
        }
        nk_layout_row_push(ctx, 0.02f);
        nk_spacer(ctx);
        nk_layout_row_push(ctx, 0.16f);
        if (nk_button_label(ctx, "Next")) {
            data.language_index = (data.language_index + 1) % LANGUAGES.size();
            data.current_progress = (data.language_index + 1) * PROGRESS_STEP;
            nk_cairo_damage(data.cairo_ctx);
        }
        nk_layout_row_push(ctx, 0.33f);
        nk_spacer(ctx);
        nk_layout_row_end(ctx);
    }
    nk_end(ctx);
}

int main(int argc, const char * argv[])
{
    @autoreleasepool {
        AppData data{};
        data.running = true;
        // Desired Nuklear draw size (pixels) — match this to the Cocoa window content
        data.draw_width = 800;   // pixels
        data.draw_height = 600;  // pixels
        // We'll use the draw size as the backing buffer size so the Nuklear UI
        // area maps 1:1 to the native window content size.
        data.width = data.draw_width;
        data.height = data.draw_height;
        data.bpp = 4;
        data.language_index = 0;
        data.current_progress = (data.language_index + 1) * PROGRESS_STEP;

        // Determine screen backing scale (may be 2.0 for Retina) and compute
        // the window size in points so the content area in pixels equals draw size.
        // Force a scale of 1.0 so the native Cocoa window points == Cairo pixels
        double scale = 1.0;
        int window_point_w = (int)ceil((double)data.draw_width / scale);
        int window_point_h = (int)ceil((double)data.draw_height / scale);

        // Create application and window
        NSApplication *app = [NSApplication sharedApplication];
        // Ensure the app has a regular activation policy so its windows appear
        [app setActivationPolicy:NSApplicationActivationPolicyRegular];
        NSRect frame = NSMakeRect(0, 0, window_point_w, window_point_h);
        // Use a fixed-size window (no resizing) to avoid scaling complications
        NSUInteger style = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable;
        NSWindow *window = [[NSWindow alloc] initWithContentRect:frame styleMask:style backing:NSBackingStoreBuffered defer:NO];
        [window setTitle:@"Atirgul"]; 

        CairoView *view = [[CairoView alloc] initWithFrame:frame appData:&data];
        [window setContentView:view];
        [window center];
        [window makeKeyAndOrderFront:nil];
        // Activate the app so the window is brought to the foreground
        [app activateIgnoringOtherApps:YES];
        fprintf(stderr, "Created window and content view\n");

        // Allocate buffer using the Nuklear draw pixel dimensions
        data.buffer_size = (size_t)data.width * (size_t)data.height * (size_t)data.bpp;
        data.buffer = (uint8_t*)malloc(data.buffer_size);
        if (!data.buffer) {
            fprintf(stderr, "Failed to allocate buffer\n");
            return -1;
        }
        memset(data.buffer, 0, data.buffer_size);

        data.cairo_ctx = nk_cairo_init(data.buffer, data.width, data.height, data.bpp, NK_CARIO_ROTATE_0);
        if (!data.cairo_ctx) {
            fprintf(stderr, "Failed to init nk cairo\n");
            free(data.buffer);
            return -1;
        }

        struct nk_context *nk_ctx = nk_cairo_get_nk_context(data.cairo_ctx);
        if (!nk_ctx) {
            fprintf(stderr, "Failed to get nk context\n");
            nk_cairo_deinit(data.cairo_ctx);
            free(data.buffer);
            return -1;
        }

        data.title_font = nk_cairo_get_font(data.cairo_ctx, "Arial", 20.0f);
        data.desc_font = nk_cairo_get_font(data.cairo_ctx, "Arial", 14.0f);

        // Simple run loop timer: 60 FPS
        // Capture a pointer to mutable AppData so the block can modify fields via pointer
        AppData *appDataPtr = &data;

        NSTimer *timer = [NSTimer scheduledTimerWithTimeInterval:(1.0/60.0) repeats:YES block:^(NSTimer * _Nonnull t) {
            // Input begin
            nk_input_begin(nk_ctx);

            // Provide mouse motion/button state
            nk_input_motion(nk_ctx, (int)last_mouse_x, (int)last_mouse_y);
            nk_input_button(nk_ctx, NK_BUTTON_LEFT, (int)last_mouse_x, (int)last_mouse_y, mouse_down);

            nk_input_end(nk_ctx);

            // Build UI and render into buffer
            draw_multilingual_window(nk_ctx, *appDataPtr);
            bool rendered = nk_cairo_render(data.cairo_ctx);

            // nk_cairo_render returns false when there's no change to draw
            // (and the backend chose to skip repaint). Treat that as a no-op,
            // and only update the CGImage / view when rendering happened.
            if (rendered) {
                [view updateImage];
                [view setNeedsDisplay:YES];
            }
        }];

        // Basic event handlers: map NSEvents to simple mouse state
        // Track mouse movement and button state via NSEvent monitoring
        [NSEvent addLocalMonitorForEventsMatchingMask:(NSEventMaskLeftMouseDown | NSEventMaskLeftMouseUp | NSEventMaskMouseMoved | NSEventMaskLeftMouseDragged) handler:^NSEvent * (NSEvent *ev) {
            NSPoint location = [ev locationInWindow];
            NSRect contentFrame = window.contentView.bounds;
            // Use scale == 1.0 so point coords map directly to pixel coords
            double s = 1.0;
            last_mouse_x = location.x * s;
            last_mouse_y = (contentFrame.size.height - location.y) * s; // flip Y for our drawing coordinate

            if (ev.type == NSEventTypeLeftMouseDown) mouse_down = true;
            if (ev.type == NSEventTypeLeftMouseUp) mouse_down = false;

            return ev;
        }];

        // Create and attach a WindowDelegate to manage close/resize and retain it
        WindowDelegate *wndDelegate = [[WindowDelegate alloc] initWithAppData:appDataPtr timer:timer view:view];
        [window setDelegate:wndDelegate];
        // Retain delegate by associating it with the window object
        objc_setAssociatedObject(window, "_atirgul_window_delegate", wndDelegate, OBJC_ASSOCIATION_RETAIN_NONATOMIC);

        // Run the app
        [app run];

        // Cleanup (will run when app terminates)
        nk_cairo_deinit(data.cairo_ctx);
        free(data.buffer);
        if (data.title_font) nk_cairo_put_font(data.title_font);
        if (data.desc_font) nk_cairo_put_font(data.desc_font);

        // invalidate timer
        [timer invalidate];
    }
    return 0;
}
