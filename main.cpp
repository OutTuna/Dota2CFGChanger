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

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <objbase.h>
#pragma comment(lib, "ole32.lib")
#endif

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
    GLFWwindow* window = glfwCreateWindow(720, 600, "Dota 2 CFG Changer", NULL, NULL);
    if (!window) return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

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

    while (!glfwWindowShouldClose(window) && !updates_should_exit()) {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImVec2 ds = io.DisplaySize;

        ui_render_main(font_big, ds);
        ui_render_success_popup(font_big, ds, io.DeltaTime);
        ui_render_update_popup();
        ui_render_backups_popup();

        ImGui::Render();
        int dw, dh;
        glfwGetFramebufferSize(window, &dw, &dh);
        glViewport(0, 0, dw, dh);
        glClearColor(0.09f, 0.09f, 0.10f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
        if (!startup_confirmed) {
            update_install_confirm_started(arguments);
            startup_confirmed = true;
        }
        if (updates_snapshot().phase == UpdatePhase::Downloaded) updates_install();
    }

    updates_shutdown();
    app_shutdown();
    avatar_shutdown();
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