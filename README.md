# Dota 2 Config Manager

Copy your Dota 2 settings from one Steam account to another. Pick the account with the setup you want, choose the destination, and confirm the copy.

[Download](https://github.com/OutTuna/Dota2CFGChanger/releases) · [Issues](https://github.com/OutTuna/Dota2CFGChanger/issues) · [Roadmap](TODO.md) · [Русский](#русский)

## Getting started

Download `DotaManager.exe` for Windows or `Dota2_CFG_Changer-x86_64.AppImage` for Linux from the releases page. Themes and translations are built in; there is no installer or extra resource folder to download.

On Linux, make the AppImage executable before opening it:

```sh
chmod +x Dota2_CFG_Changer-x86_64.AppImage
./Dota2_CFG_Changer-x86_64.AppImage
```

Both platforms need a working OpenGL driver. The Linux folder picker uses `zenity` or `kdialog`, and the interface needs a system font such as DejaVu, Liberation or Noto. macOS is not supported.

1. Close Dota 2 and Steam before copying.
2. Check the source and destination folders. The app tries to find Steam's `userdata` directory, including Linux Flatpak installations. You can choose another folder manually.
3. Click **Scan folders**.
4. Select the source account on the left and the destination account on the right.
5. Click **Copy config** and confirm.

The folders should follow Steam's layout: `<root>/<account_id>/570/`. You can use the same `userdata` root for both lists, or a saved copy as the source. Only the selected account's `570` directory is replaced.

**Keep a separate backup of any settings you want to keep.** The app prepares the new files before replacing the destination and attempts to restore the old directory if the replacement fails. After a successful copy, the temporary backup is removed. If interrupted files remain, resolve them before trying again; the app will show the affected paths.

## A few details

- English is the default language. Russian and Ukrainian are available in settings.
- Five themes are included. The Crimson theme also has a palette editor. A local `themes/` folder can override the built-in themes; missing or invalid resources fall back to the embedded ones.
- Account names and avatars come from public Steam Community profiles and are cached locally. This needs an internet connection, but no Steam login or API key. If a profile lookup fails, the account ID still appears in the list.
- Release builds check for updates at startup. You can also check using the button at the bottom of the window. Downloading starts only when you click **Download**; the app checks the file size and SHA-256 before keeping it. If a verified download is unavailable, use **Open release**. Launch the new file yourself after closing the old version.

Settings, profile caches, avatars and downloaded updates live in `%APPDATA%\DotaManager` on Windows, or `$XDG_CONFIG_HOME/DotaManager` / `~/.config/DotaManager` on Linux.

## Building

You need Git, CMake 3.14+, a C++17 compiler and internet access for the first configuration. CMake fetches the pinned dependencies: Dear ImGui, GLFW, cpr/libcurl, nlohmann/json and stb.

On Debian or Ubuntu, install the development packages first:

```sh
sudo apt install build-essential cmake git libgl1-mesa-dev libx11-dev \
  libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libssl-dev
```

From a Visual Studio developer terminal on Windows, or a shell on Linux:

```sh
git clone https://github.com/OutTuna/Dota2CFGChanger.git
cd Dota2CFGChanger
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
```

With the Visual Studio generator, the result is `build/Release/DotaManager.exe`. On Linux it is `build/DotaManager`; the GitHub Actions workflow handles AppImage packaging. CI publishes the Windows `.exe` and Linux `.AppImage`, and caches dependencies between builds.

For a versioned local build, pass `-DAPP_VERSION=x.y` when configuring. The default `0.0` development build skips the startup update check.

To build and run the C++ checks:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DDOTAMANAGER_BUILD_TESTS=ON
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

## How the code fits together

`main.cpp` runs the window. `ui/` draws the interface, while `dotamanager_core` handles scanning, config replacement, profile data and updates without depending on ImGui or OpenGL. `avatar.cpp` uploads the images from `avatar_data.cpp` as OpenGL textures. Theme and translation resources are embedded during the build.

[Open the full diagram in GitDiagram](https://gitdiagram.com/outtuna/dota2cfgchanger).

<details>
<summary>Project diagram</summary>

```mermaid
flowchart TB
    node_main_window["Main window<br/>main_window.cpp"]
    node_panels["Settings and confirmation<br/>panels.cpp"]
    node_success_popup["Copy success popup<br/>success_popup.cpp"]
    node_update_popup["Update popup<br/>update_popup.cpp"]
    node_main["Application entry<br/>main.cpp"]
    node_app["App coordinator<br/>app.cpp"]
    node_file_ops["Config replacement<br/>file_ops.cpp"]
    node_platform["Platform services<br/>platform.cpp"]
    node_steam_api["Profile lookup<br/>steam_api.cpp"]
    node_avatar_data["Avatar downloads<br/>avatar_data.cpp"]
    node_avatar["Avatar rendering<br/>avatar.cpp"]
    node_theme["Theme system<br/>theme.cpp"]
    node_theme_resources["Theme resources"]
    node_localization["Localization<br/>localization.cpp"]
    node_settings_cache["Settings and caches<br/>app.cpp"]
    node_updates["Release updates<br/>updates.cpp"]
    node_update_transport["Update transport"]
    node_release_info["Release metadata<br/>release_info.cpp"]
    node_user["User"]
    node_steam_data["Steam userdata"]
    node_steam_community["Steam Community"]
    node_release_service["Release service"]
    node_user --> node_main_window
    node_main --> node_app
    node_main --> node_main_window
    node_main --> node_updates
    node_main --> node_avatar
    node_main_window --> node_app
    node_main_window --> node_panels
    node_panels --> node_app
    node_app --> node_steam_data
    node_app --> node_steam_api
    node_steam_api --> node_steam_community
    node_app --> node_file_ops
    node_file_ops --> node_steam_data
    node_app --> node_settings_cache
    node_avatar_data --> node_app
    node_avatar --> node_avatar_data
    node_main_window --> node_avatar
    node_main_window --> node_theme
    node_panels --> node_localization
    node_theme --> node_theme_resources
    node_updates --> node_update_transport
    node_updates --> node_release_info
    node_update_transport -.-> node_release_service
    node_main --> node_update_popup
    node_update_popup --> node_updates
    node_main_window --> node_platform
    node_app --> node_localization
```

</details>

## Русский

Программа переносит настройки Dota 2 между Steam-аккаунтами. Скачайте `.exe` для Windows или `.AppImage` для Linux из [релизов](https://github.com/OutTuna/Dota2CFGChanger/releases). Темы и переводы уже внутри.

Закройте Dota 2 и Steam, проверьте пути к `userdata` и запустите сканирование. Слева выберите аккаунт с нужными настройками, справа — тот, куда их перенести. Нажмите кнопку копирования и подтвердите замену.

Копируется папка `<account_id>/570` целиком. Если старые настройки нужны, сохраните их отдельно: после успешной замены временная резервная копия удаляется.

Язык и тема меняются в настройках. Имена и аватарки загружаются из Steam Community без входа в аккаунт. При наличии обновления появится окно; файл скачивается только по нажатию. После загрузки закройте старую версию и запустите новую.

## License

[MIT](LICENSE). OutTuna.
