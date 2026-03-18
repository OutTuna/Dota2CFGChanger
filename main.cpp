#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <vector>
#include <string>
#include <thread>
#include <map>
#include <algorithm>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <cstdlib>
#endif

namespace fs = std::filesystem;
using json = nlohmann::json;

const std::string SETTINGS_FILE = "settings.json";
const std::string CACHE_FILE = "nicknames.json";
const std::string DOTA_ID = "570";

char src_path[256] = "";
char dst_path[256] = "";
std::map<std::string, std::string> nick_cache;
std::vector<std::string> src_list;
std::vector<std::string> dst_list;
std::string status_msg = "Ready";
int selected_src = -1;
int selected_dst = -1;

std::string get_default_steam_path() {
#ifdef _WIN32
    return "C:\\Program Files (x86)\\Steam\\userdata";
#else
    const char* home = getenv("HOME");
    if (home) {
        std::string p1 = std::string(home) + "/.local/share/Steam/userdata";
        if (fs::exists(p1)) return p1;
        // Flatpak Steam
        std::string p2 = std::string(home) + "/.var/app/com.valvesoftware.Steam/data/Steam/userdata";
        if (fs::exists(p2)) return p2;
        // Snap Steam
        std::string p3 = std::string(home) + "/snap/steam/common/.local/share/Steam/userdata";
        if (fs::exists(p3)) return p3;
        return p1;
    }
    return "";
#endif
}

std::string get_font_path() {
#ifdef _WIN32
    return "C:\\Windows\\Fonts\\arial.ttf";
#else
    std::vector<std::string> candidates = {
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/TTF/DejaVuSans.ttf",
        "/usr/share/fonts/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/ubuntu/Ubuntu-R.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
        "/usr/share/fonts/liberation/LiberationSans-Regular.ttf",
    };
    for (const auto& p : candidates) {
        if (fs::exists(p)) return p;
    }
    return "";
#endif
}

void load_settings() {
    if (strlen(dst_path) == 0) {
        std::string def = get_default_steam_path();
        strncpy(dst_path, def.c_str(), sizeof(dst_path));
        dst_path[sizeof(dst_path) - 1] = 0;
    }

    if (fs::exists(SETTINGS_FILE)) {
        try {
            std::ifstream f(SETTINGS_FILE);
            json j; f >> j;
            std::string s = j.value("src", "");
            std::string d = j.value("dst", "");
            if (!s.empty()) {
                strncpy(src_path, s.c_str(), sizeof(src_path));
                src_path[sizeof(src_path) - 1] = 0;
            }
            if (!d.empty()) {
                strncpy(dst_path, d.c_str(), sizeof(dst_path));
                dst_path[sizeof(dst_path) - 1] = 0;
            }
        } catch(...) {}
    }

    if (fs::exists(CACHE_FILE)) {
        try {
            std::ifstream f(CACHE_FILE);
            json j; f >> j;
            for (auto& el : j.items()) nick_cache[el.key()] = el.value();
        } catch(...) {}
    }
}

void save_settings() {
    std::ofstream f(SETTINGS_FILE);
    json j = {{"src", src_path}, {"dst", dst_path}};
    f << j;
    std::ofstream fc(CACHE_FILE);
    json jc(nick_cache);
    fc << jc;
}

std::string clean_xml_nick(std::string raw) {
    std::string start_tag = "<![CDATA[";
    size_t pos = raw.find(start_tag);
    if (pos != std::string::npos) {
        raw.replace(pos, start_tag.length(), "");
    }
    std::string end_tag = "]]>";
    pos = raw.find(end_tag);
    if (pos != std::string::npos) {
        raw.replace(pos, end_tag.length(), "");
    }
    return raw;
}

std::string fetch_nick(std::string id) {
    if (nick_cache.count(id)) return nick_cache[id];
    try {
        long long steam64 = std::stoll(id) + 76561197960265728LL;
        std::string url = "https://steamcommunity.com/profiles/" + std::to_string(steam64) + "?xml=1";
        cpr::Response r = cpr::Get(cpr::Url{url}, cpr::Timeout{2000});

        if (r.status_code == 200) {
            size_t start = r.text.find("<steamID>");
            size_t end   = r.text.find("</steamID>");
            if (start != std::string::npos && end != std::string::npos) {
                std::string nick = r.text.substr(start + 9, end - start - 9);
                nick = clean_xml_nick(nick);
                nick_cache[id] = nick;
                return nick;
            }
        }
    } catch (...) {}
    return "ID: " + id;
}

void scan_thread() {
    status_msg = "Scanning...";
    save_settings();
    src_list.clear();
    dst_list.clear();
    std::vector<std::string> all_ids;

    auto scan_dir = [&](std::string path, std::vector<std::string>& list) {
        if (fs::exists(path)) {
            for (const auto& entry : fs::directory_iterator(path)) {
                if (entry.is_directory()) {
                    std::string fname = entry.path().filename().string();
                    if (std::all_of(fname.begin(), fname.end(), ::isdigit)) {
                        all_ids.push_back(fname);
                        list.push_back(fname);
                    }
                }
            }
        }
    };

    scan_dir(src_path, src_list);
    scan_dir(dst_path, dst_list);

    for (const auto& id : all_ids) {
        if (nick_cache.find(id) == nick_cache.end()) {
            fetch_nick(id);
        }
    }
    save_settings();
    status_msg = "Scan Complete!";
}

void copy_config() {
    if (selected_src < 0 || selected_dst < 0) {
        status_msg = "Select folders first!";
        return;
    }
    if (selected_src >= (int)src_list.size() || selected_dst >= (int)dst_list.size()) return;

    std::string s_id = src_list[selected_src];
    std::string d_id = dst_list[selected_dst];

    fs::path src = fs::path(src_path) / s_id / DOTA_ID;
    fs::path dst = fs::path(dst_path) / d_id / DOTA_ID;

    if (!fs::exists(src)) {
        status_msg = "No Dota config in source!";
        return;
    }

    try {
        if (fs::exists(dst)) fs::remove_all(dst);
        fs::copy(src, dst, fs::copy_options::recursive);
        status_msg = "Success! Copied to " + d_id;
    } catch (std::exception& e) {
        status_msg = "Error: " + std::string(e.what());
    }
}

int main(int, char**) {
    load_settings();

    if (!glfwInit()) return 1;

    GLFWwindow* window = glfwCreateWindow(700, 500, "Dota 2 Manager C++", NULL, NULL);
    if (window == NULL) return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    ImGuiIO& io = ImGui::GetIO();

    std::string font_path = get_font_path();
    ImFont* font = nullptr;
    if (!font_path.empty()) {
        font = io.Fonts->AddFontFromFileTTF(
            font_path.c_str(), 16.0f, NULL,
            io.Fonts->GetGlyphRangesCyrillic()
        );
    }
    (void)font;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
        ImGui::Begin("Main", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize);

        ImGui::TextColored(ImVec4(1, 0, 0, 1), "DOTA 2 C++ MANAGER");
        ImGui::Separator();

        ImGui::InputText("Source Path", src_path, 256);
        ImGui::InputText("Dest Path",   dst_path, 256);

        if (ImGui::Button("SCAN FOLDERS", ImVec2(-1, 40))) {
            std::thread(scan_thread).detach();
        }

        ImGui::Columns(2, "lists", true);
        ImGui::Text("From (Config)");

        auto getter = [](void* data, int idx, const char** out_text) -> bool {
            auto& vec = *static_cast<std::vector<std::string>*>(data);
            if (idx < 0 || idx >= (int)vec.size()) return false;
            std::string id = vec[idx];
            static std::string buf;
            if (nick_cache.count(id))
                buf = nick_cache[id] + " (" + id + ")";
            else
                buf = id;
            *out_text = buf.c_str();
            return true;
        };

        ImGui::ListBox("##src", &selected_src, getter, &src_list, (int)src_list.size(), 12);
        ImGui::NextColumn();

        ImGui::Text("To (Account)");
        ImGui::ListBox("##dst", &selected_dst, getter, &dst_list, (int)dst_list.size(), 12);
        ImGui::Columns(1);

        ImGui::Separator();
        if (ImGui::Button("COPY CONFIG NOW", ImVec2(-1, 50))) {
            copy_config();
        }
        ImGui::Text("Status: %s", status_msg.c_str());

        ImGui::End();
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

#ifdef _WIN32
int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdline, int cmdshow) {
    return main(__argc, __argv);
}
#endif