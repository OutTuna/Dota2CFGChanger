#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include "app_logic.h"
#include "avatar.h"
#include "ui.h"
#include "app_icon.h"
#include "app_state.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <objbase.h>
#pragma comment(lib, "ole32.lib")
#endif

int main(int, char**) {
    load_settings();

#ifdef _WIN32
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
#endif

    if (!glfwInit()) return 1;
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(720, 560, "Dota 2 CFG Changer", NULL, NULL);
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

    ui_apply_theme(static_cast<AppTheme>(g_theme));

    static const char* kRegularFontCandidates[] = {
#ifdef _WIN32
        "C:\\Windows\\Fonts\\arial.ttf",
        "C:\\Windows\\Fonts\\segoeui.ttf",
#elif defined(__APPLE__)
        "/System/Library/Fonts/Supplemental/Arial.ttf",
        "/Library/Fonts/Arial.ttf",
#else
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
        "/usr/share/fonts/TTF/DejaVuSans.ttf",
#endif
    };
    static const char* kBoldFontCandidates[] = {
#ifdef _WIN32
        "C:\\Windows\\Fonts\\arialbd.ttf",
        "C:\\Windows\\Fonts\\segoeuib.ttf",
        "C:\\Windows\\Fonts\\arial.ttf",
#elif defined(__APPLE__)
        "/System/Library/Fonts/Supplemental/Arial Bold.ttf",
        "/Library/Fonts/Arial Bold.ttf",
        "/System/Library/Fonts/Supplemental/Arial.ttf",
#else
        "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Bold.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
#endif
    };

    ImGuiIO& io = ImGui::GetIO();

    auto load_first_available = [&](const char* const* candidates, size_t count, float size) -> ImFont* {
        for (size_t i = 0; i < count; ++i) {
            ImFont* f = io.Fonts->AddFontFromFileTTF(
                candidates[i], size, NULL, io.Fonts->GetGlyphRangesCyrillic());
            if (f) return f;
        }
        return nullptr;
    };

    ImFont* font_default = load_first_available(
        kRegularFontCandidates,
        sizeof(kRegularFontCandidates) / sizeof(kRegularFontCandidates[0]),
        16.0f);
    ImFont* font_big = load_first_available(
        kBoldFontCandidates,
        sizeof(kBoldFontCandidates) / sizeof(kBoldFontCandidates[0]),
        34.0f);

    if (!font_default) font_default = io.Fonts->AddFontDefault();
    if (!font_big)      font_big     = font_default;
    (void)font_default;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImVec2 ds = io.DisplaySize;

        ui_render_main(font_big, ds);
        ui_render_success_popup(font_big, ds, io.DeltaTime);

        ImGui::Render();
        int dw, dh;
        glfwGetFramebufferSize(window, &dw, &dh);
        glViewport(0, 0, dw, dh);
        glClearColor(0.09f, 0.09f, 0.10f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

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
