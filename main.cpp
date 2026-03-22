#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include "app_logic.h"
#include "ui.h"
#include "app_icon.h"

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
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    ui_apply_theme();

    ImGuiIO& io = ImGui::GetIO();
    ImFont* font_default = io.Fonts->AddFontFromFileTTF(
        "C:\\Windows\\Fonts\\arial.ttf", 16.0f, NULL,
        io.Fonts->GetGlyphRangesCyrillic());
    (void)font_default;

    ImFont* font_big = io.Fonts->AddFontFromFileTTF(
        "C:\\Windows\\Fonts\\arialbd.ttf", 34.0f, NULL,
        io.Fonts->GetGlyphRangesCyrillic());
    if (!font_big)
        font_big = io.Fonts->AddFontFromFileTTF(
            "C:\\Windows\\Fonts\\arial.ttf", 34.0f, NULL,
            io.Fonts->GetGlyphRangesCyrillic());

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