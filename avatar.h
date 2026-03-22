#pragma once
#include <string>
#include <imgui.h>

ImTextureID avatar_get(const std::string& steam3_id);
void avatar_flush_pending();
void avatar_shutdown();