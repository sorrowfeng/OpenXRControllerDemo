/*
 * Copyright 2024 - 2024 PICO. All rights reserved.
 *
 * NOTICE: All information contained herein is, and remains the property of PICO.
 * The intellectual and technical concepts contained herein are proprietary to PICO.
 * and may be covered by patents, patents in process, and are protected by trade
 * secret or copyright law. Dissemination of this information or reproduction of
 * this material is strictly forbidden unless prior written permission is obtained
 * from PICO.
 */

#ifndef PICONATIVEOPENXRSAMPLES_IMGUIRENDERER_H
#define PICONATIVEOPENXRSAMPLES_IMGUIRENDERER_H

#include "IGuiRenderer.h"
#include "imgui.h"

#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <thread>
#include <map>

namespace PVRSampleFW {

    enum GuiFontSize {
        FONT_SIZE_16 = 0,
        FONT_SIZE_20,
        FONT_SIZE_24,
        FONT_SIZE_28,
        FONT_SIZE_32,
        FONT_SIZE_36 = 5,
        FONT_SIZE_40,
        FONT_SIZE_NUM
    };

    struct GuiGLContext {
        EGLDisplay display{EGL_NO_DISPLAY};
        EGLContext context{EGL_NO_CONTEXT};
    };

    struct RenderTarget {
        GLsizei width{0};
        GLsizei height{0};
        GLuint textureId{0};
        GLuint framebufferId{0};
        ImGuiContext *imGuiContext{nullptr};
        ImFont *font[FONT_SIZE_NUM]{nullptr};
    };

    class ImGuiRenderer : public IGuiRenderer {
    public:
        static ImGuiRenderer *GetInstance() {
            static ImGuiRenderer instance;
            return &instance;
        }

        ~ImGuiRenderer() override;

        void Initialize(void *context) override;

        void Shutdown() override;

        void TriggerSignal() override;

        uint32_t AddWindow(const std::shared_ptr<GuiWindow> &window) override;

    private:
        ImGuiRenderer() = default;

        ImGuiRenderer(const ImGuiRenderer &) = delete;

        ImGuiRenderer &operator=(const ImGuiRenderer &) = delete;

        int InitForContext(EGLDisplay display, EGLContext context);

        void ConfigRenderTarget(RenderTarget *renderTarget, const WindowConfig &config);

        void checkNewFont(RenderTarget *renderTarget, int fontSize);

        ImFont *GetFont(RenderTarget *renderTarget, int fontSize);

        void RenderAllWindows();

        void RenderOneWindow(const std::shared_ptr<GuiWindow> &window, RenderTarget *renderTarget);

        void ConfigImGuiDisplay(uint32_t width, uint32_t height);

        void InitForMultiThread();

        void Destroy();

    private:
        static constexpr int POINT_SIZE{4};
        static constexpr int SWAP_BUFFER_COUNT{1};
        static constexpr int TEXT_WIDTH_PIXEL{600};
        static constexpr int MIN_FONT_SIZE{16};
        static constexpr int MAX_FONT_SIZE{40};
        static constexpr int FONT_SIZE_INTERVAL{4};
        static constexpr int DEFAULT_FONT_SIZE{24};
        static constexpr float HORIZONTAL_MARGIN_PIXEL{100.0f};
        static constexpr float VERTICAL_MARGIN_PIXEL{100.0f};

        bool initialized_{false};
        bool render_in_xr_loop_{true};
        EGLDisplay display_{EGL_NO_DISPLAY};
        EGLContext context_{EGL_NO_CONTEXT};

        std::thread thread_;
        std::condition_variable condition_;
        std::mutex mutex_;

        std::map<uint32_t, std::shared_ptr<GuiWindow>> gui_window_map_;
        std::map<uint32_t, RenderTarget> render_target_map_;
        uint32_t window_num_{0};
        int default_font_size_{DEFAULT_FONT_SIZE};
    };

}  // namespace PVRSampleFW

#endif  //PICONATIVEOPENXRSAMPLES_IMGUIRENDERER_H
