#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <objbase.h>
#include <mmsystem.h>
#pragma comment(lib, "ole32.lib")
#endif
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include "app.h"
#include "avatar.h"
#include "ui.h"
#include "app_icon.h"
#include "updates.h"
#include "update_install.h"
#include "platform/update_process.h"
#include "ui_events.h"
#include <algorithm>

namespace {
double input_until = 0;
void input_changed() {
    input_until = glfwGetTime() + 0.75;
    ui_events::request_redraw();
}
}

namespace {
class FrameTimerResolution {
public:
    void set_active(bool active) {
#ifdef _WIN32
        if (active == enabled) return;
        if (active) enabled = timeBeginPeriod(1) == TIMERR_NOERROR;
        else { timeEndPeriod(1); enabled = false; }
#else
        (void)active;
#endif
    }
    ~FrameTimerResolution() { set_active(false); }
private:
    bool enabled = false;
};
}

int main(int argc, char** argv) {
    auto arguments = update_arguments(argc, argv);
    int helper_result = update_helper_dispatch(arguments);
    if (helper_result >= 0) return helper_result;
    load_settings();

#ifdef _WIN32
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
#endif

    if (!glfwInit()) return 1;
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
#ifdef __linux__
    glfwWindowHintString(GLFW_X11_CLASS_NAME, "DotaManager");
#endif
    const std::string window_title = std::string("Dota 2 CFG Changer (v") + app_version() + ")";
    GLFWwindow* window = glfwCreateWindow(720, 600, window_title.c_str(), NULL, NULL);
    if (!window) return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    ui_events::set_wake(glfwPostEmptyEvent);
    glfwSetCursorPosCallback(window, [](GLFWwindow*, double, double) { input_changed(); });
    glfwSetCursorEnterCallback(window, [](GLFWwindow*, int) { input_changed(); });
    glfwSetMouseButtonCallback(window, [](GLFWwindow*, int, int, int) { input_changed(); });
    glfwSetScrollCallback(window, [](GLFWwindow*, double, double) { input_changed(); });
    glfwSetKeyCallback(window, [](GLFWwindow*, int, int, int, int) { input_changed(); });
    glfwSetCharCallback(window, [](GLFWwindow*, unsigned int) { input_changed(); });
    glfwSetWindowFocusCallback(window, [](GLFWwindow*, int) { input_changed(); });
    glfwSetWindowRefreshCallback(window, [](GLFWwindow*) { ui_events::request_redraw(); });
    glfwSetFramebufferSizeCallback(window, [](GLFWwindow*, int, int) { ui_events::request_redraw(); });
    glfwSetWindowIconifyCallback(window, [](GLFWwindow*, int) { input_changed(); });

    GLFWimage icon;
    icon.width  = APP_ICON_WIDTH;
    icon.height = APP_ICON_HEIGHT;
    icon.pixels = (unsigned char*)APP_ICON_RGBA;
    glfwSetWindowIcon(window, 1, &icon);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().IniFilename = nullptr;
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    ui_apply_theme(static_cast<AppTheme>(g_theme.load()));

    ImGuiIO& io = ImGui::GetIO();

    FontPaths fonts = find_font_paths();
    ImFont* font_default = nullptr;
    ImFont* font_big = nullptr;

    if (!fonts.regular.empty()) {
        font_default = io.Fonts->AddFontFromFileTTF(
            fonts.regular.c_str(), 16.0f, NULL, io.Fonts->GetGlyphRangesCyrillic());
    }
    if (!fonts.bold.empty()) {
        font_big = io.Fonts->AddFontFromFileTTF(
            fonts.bold.c_str(), 28.0f, NULL, io.Fonts->GetGlyphRangesCyrillic());
    }

    if (!font_default) font_default = io.Fonts->AddFontDefault();
    if (!font_big)     font_big     = font_default;
    (void)font_default;

    auto install_error = update_install_startup_error(arguments);
    if (install_error.empty()) updates_check(false);
    else updates_install_error(install_error);
    bool startup_confirmed = false;
    double next_frame = 0;
    int settle_frames = 2;
    bool last_focus = true;
    FrameTimerResolution frame_timer;

    while (!glfwWindowShouldClose(window) && !updates_should_exit()) {
        glfwPollEvents();
        if (glfwWindowShouldClose(window) || updates_should_exit()) break;
        if (updates_phase() == UpdatePhase::Downloaded) updates_install();
        int dw, dh;
        glfwGetFramebufferSize(window, &dw, &dh);
        if (glfwGetWindowAttrib(window, GLFW_ICONIFIED) || dw == 0 || dh == 0) {
            frame_timer.set_active(false);
            if (ui_events::dirty.exchange(false)) continue;
            glfwWaitEvents();
            continue;
        }
        bool focused = glfwGetWindowAttrib(window, GLFW_FOCUSED) != 0;
        bool held_input = false;
        for (bool down : io.MouseDown) held_input |= down;
        for (const auto& key : io.KeysData) held_input |= key.Down;
        const double now = glfwGetTime();
        if (focused != last_focus) { next_frame = now; last_focus = focused; }
        const bool continuous = g_success.show ||
            (focused && (now < input_until || held_input || io.WantTextInput));
        if (!ui_events::dirty.load() && settle_frames == 0 && !continuous) {
            frame_timer.set_active(false);
            glfwWaitEvents();
            continue;
        }
        frame_timer.set_active(focused);
        if (now < next_frame) {
            glfwWaitEventsTimeout(std::max(0.001, next_frame - now));
            continue;
        }
        if (ui_events::dirty.exchange(false)) settle_frames = 2;
        if (settle_frames > 0) --settle_frames;
        next_frame = now + (focused ? 1.0 / 60.0 : 1.0 / 5.0);
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        io.DeltaTime = std::min(io.DeltaTime, 0.1f);
        ImGui::NewFrame();

        ImVec2 ds = io.DisplaySize;

        ui_render_main(font_big, ds);
        ui_render_success_popup(font_big, ds, io.DeltaTime);
        ui_render_update_popup();
        ui_render_backups_popup();

        ImGui::Render();
        glfwGetFramebufferSize(window, &dw, &dh);
        if (dw == 0 || dh == 0 || glfwGetWindowAttrib(window, GLFW_ICONIFIED)) continue;
        glViewport(0, 0, dw, dh);
        glClearColor(0.09f, 0.09f, 0.10f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
        if (!startup_confirmed) {
            update_install_confirm_started(arguments);
            startup_confirmed = true;
        }
    }

    frame_timer.set_active(false);
    updates_shutdown();
    app_shutdown();
    avatar_shutdown();
    ui_events::set_wake(nullptr);
    ui_shutdown_theme();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();

#ifdef _WIN32
    CoUninitialize();
#endif
    return 0;
}

#ifdef _WIN32
int APIENTRY WinMain(HINSTANCE, HINSTANCE, PSTR, int) { return main(__argc, __argv); }
#endif
