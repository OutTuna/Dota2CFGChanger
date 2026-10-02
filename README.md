<p align="center">
  <img src="resources/icons/dotamanager.png" width="64" height="64" alt="Dota 2 Config Manager icon">
</p>

<h1 align="center">Dota 2 Config Manager</h1>

<p align="center">Your Dota 2 setup, on another Steam account.</p>

<p align="center">
  <img src="docs/assets/readme-divider.svg" width="640" height="6" alt="">
</p>

<p align="center">
  <a href="https://github.com/OutTuna/Dota2CFGChanger/actions/workflows/build.yaml"><img src="https://img.shields.io/github/actions/workflow/status/OutTuna/Dota2CFGChanger/build.yaml?branch=main&amp;style=for-the-badge&amp;label=build" alt="Build status"></a>
  <a href="https://github.com/OutTuna/Dota2CFGChanger/releases/latest"><img src="https://img.shields.io/badge/download-Latest-c95050?style=for-the-badge" alt="Download Latest"></a>
  <a href="#getting-started"><img src="https://img.shields.io/badge/platforms-Windows%20%7C%20Linux-586575?style=for-the-badge" alt="Windows and Linux"></a>
  <a href="#a-few-details"><img src="https://img.shields.io/badge/languages-EN%20%7C%20RU%20%7C%20UA-586575?style=for-the-badge" alt="English, Russian and Ukrainian"></a>
  <a href="LICENSE"><img src="https://img.shields.io/badge/license-MIT-586575?style=for-the-badge" alt="MIT license"></a>
</p>

<p align="center">
  <a href="https://github.com/OutTuna/Dota2CFGChanger/releases/latest">Download</a> ·
  <a href="https://github.com/OutTuna/Dota2CFGChanger/issues">Issues</a> ·
  <a href="docs/TODO.md">Roadmap</a> ·
  <a href="#русский">Русский</a>
</p>

Copy your Dota 2 settings from one Steam account to another. Pick the account with the setup you want, choose the destination, and confirm the copy.

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

- English is the default language. Switch between **RU / UA / EN** at the top right, immediately beside the theme selector. Language and theme are saved automatically.
- Dark is the default theme; five themes are included. The Crimson theme also has a palette editor. A local `themes/` folder can override the built-in themes; missing or invalid resources fall back to the embedded ones.
- Account names and avatars come from public Steam Community profiles and are cached locally. This needs an internet connection, but no Steam login or API key. If a profile lookup fails, the account ID still appears in the list.
- Release builds check for updates at startup. You can also check using the button at the bottom of the window. Downloading starts only when you click **Download**; the app checks the file size and SHA-256 before keeping it. If a verified download is unavailable, use **Open release**. Launch the new file yourself after closing the old version.

On Windows, paths, theme and language are saved under `HKEY_CURRENT_USER\Software\OutTuna\Dota2CFGChanger`, in the `Settings` value. An existing `settings.json` is migrated and removed only after a successful registry write. Profile caches, avatars and updates still use `%APPDATA%\DotaManager`. On Linux, settings and caches use `$XDG_CONFIG_HOME/DotaManager` / `~/.config/DotaManager`.

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

With the Visual Studio generator, the result is `build/Release/DotaManager.exe`. On Linux it is `build/DotaManager`; the GitHub Actions workflow handles AppImage packaging. CI publishes the Windows `.exe` and Linux `.AppImage` as a stable Latest release, checks the compiled version and caches dependencies between builds. The AppImage is built on Ubuntu 22.04 with GCC 11, checked for Ubuntu 22.04 ABI compatibility and launched under a virtual display on Ubuntu 22.04 and 24.04 before publication. Official Actions use Node.js 24.

For a versioned local build, pass `-DAPP_VERSION=x.y` when configuring. The default `0.0` development build skips the startup update check.

To build and run the C++ checks:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DDOTAMANAGER_BUILD_TESTS=ON
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

## How the code fits together

`main.cpp` runs the window. `src/ui/` draws the interface, while `dotamanager_core` handles scanning, config replacement, profile data and updates without depending on ImGui or OpenGL. `src/ui/avatar.cpp` uploads the images from `src/core/avatar_data.cpp` as OpenGL textures. Theme and translation resources are embedded during the build.

```text
main.cpp              application entry point
src/core/             settings, scanning, config files, caches and updates
src/network/          Steam and release HTTP requests
src/platform/         native paths, dialogs, fonts and external links
src/ui/               windows, panels, themes and OpenGL avatars
resources/            icons, themes, translations and Windows metadata
cmake/                resource embedding
scripts/              development and CI checks
tests/                regression checks
docs/                 roadmap
```

The AppImage uses the checked-in PNG icon. CI does not need an image converter.

[Open the full diagram in GitDiagram](https://gitdiagram.com/outtuna/dota2cfgchanger).

<details>
<summary>Project diagram</summary>

```mermaid
flowchart TB

subgraph group_ui["User interface"]
  node_entry["App startup<br/>[main.cpp]"]
  node_window["Main window<br/>[main_window.cpp]"]
  node_panels["Dialogs and panels<br/>[panels.cpp]"]
  node_success["Copy result<br/>[success_popup.cpp]"]
  node_updateui["Update dialog<br/>[update_popup.cpp]"]
end

subgraph group_core["Application core"]
  node_app["App coordination<br/>[app.cpp]"]
  node_files["Config replacement<br/>[file_ops.cpp]"]
  node_settings[("Settings and caches<br/>[app.cpp]")]
  node_localization["Translations<br/>[localization.cpp]"]
  node_avatar_data[("Avatar cache<br/>[avatar_data.cpp]")]
  node_updates["Update service<br/>[updates.cpp]"]
  node_release["Release metadata<br/>[release_info.cpp]"]
  node_checksum["Download verification<br/>[checksum.cpp]"]
end

subgraph group_net["Network services"]
  node_steam["Steam profiles<br/>[steam_api.cpp]"]
  node_transport["Release transport"]
end

subgraph group_platform["Platform and presentation resources"]
  node_platform_api["Platform services<br/>[platform.cpp]"]
  node_avatars["Avatar textures<br/>[avatar.cpp]"]
  node_themes["Themes<br/>[theme.cpp]"]
  node_resources["Theme and locale data"]
end

node_user(("User"))
node_steamcommunity(("Steam Community"))
node_release_service(("GitHub releases"))
node_configdirs[("Steam userdata folders")]
node_local_settings[("Local settings store")]

node_user -->|"operates"| node_window
node_entry -->|"loads settings"| node_app
node_entry -->|"renders"| node_window
node_entry -->|"checks updates"| node_updates
node_window -->|"scans and copies"| node_app
node_window -->|"opens dialogs"| node_panels
node_window -->|"requests avatars"| node_avatars
node_window -->|"uses themes"| node_themes
node_window -->|"translates labels"| node_localization
node_window -->|"requests checks"| node_updates
node_app -->|"replaces config"| node_files
node_app -->|"fetches profiles"| node_steam
node_app -->|"reads and writes"| node_settings
node_app -->|"scans and copies"| node_configdirs
node_settings -->|"persists"| node_local_settings
node_app -->|"uses path services"| node_platform_api
node_steam -->|"looks up profiles"| node_steamcommunity
node_avatar_data -->|"downloads images"| node_steamcommunity
node_avatars -->|"uploads cached images"| node_avatar_data
node_updates -->|"fetches and downloads"| node_transport
node_updates -->|"verifies download"| node_checksum
node_updates -->|"opens release"| node_platform_api
node_updates -->|"uses release metadata"| node_release
node_transport -->|"requests releases"| node_release_service
node_updateui -->|"controls updates"| node_updates
node_themes -->|"loads theme data"| node_resources
node_localization -->|"uses translations"| node_resources
node_window -->|"shows result"| node_success

click node_entry "https://github.com/outtuna/dota2cfgchanger/blob/main/main.cpp"
click node_window "https://github.com/outtuna/dota2cfgchanger/blob/main/src/ui/main_window.cpp"
click node_panels "https://github.com/outtuna/dota2cfgchanger/blob/main/src/ui/panels.cpp"
click node_success "https://github.com/outtuna/dota2cfgchanger/blob/main/src/ui/success_popup.cpp"
click node_updateui "https://github.com/outtuna/dota2cfgchanger/blob/main/src/ui/update_popup.cpp"
click node_app "https://github.com/outtuna/dota2cfgchanger/blob/main/src/core/app.cpp"
click node_files "https://github.com/outtuna/dota2cfgchanger/blob/main/src/core/file_ops.cpp"
click node_settings "https://github.com/outtuna/dota2cfgchanger/blob/main/src/core/app.cpp"
click node_localization "https://github.com/outtuna/dota2cfgchanger/blob/main/src/core/localization.cpp"
click node_avatar_data "https://github.com/outtuna/dota2cfgchanger/blob/main/src/core/avatar_data.cpp"
click node_updates "https://github.com/outtuna/dota2cfgchanger/blob/main/src/core/updates.cpp"
click node_release "https://github.com/outtuna/dota2cfgchanger/blob/main/src/core/release_info.cpp"
click node_checksum "https://github.com/outtuna/dota2cfgchanger/blob/main/src/core/checksum.cpp"
click node_steam "https://github.com/outtuna/dota2cfgchanger/blob/main/src/network/steam_api.cpp"
click node_transport "https://github.com/outtuna/dota2cfgchanger/blob/main/src/network/update_transport.cpp"
click node_platform_api "https://github.com/outtuna/dota2cfgchanger/blob/main/src/platform/platform.cpp"
click node_avatars "https://github.com/outtuna/dota2cfgchanger/blob/main/src/ui/avatar.cpp"
click node_themes "https://github.com/outtuna/dota2cfgchanger/blob/main/src/ui/theme.cpp"
click node_resources "https://github.com/outtuna/dota2cfgchanger/tree/main/resources"

classDef toneNeutral fill:#f8fafc,stroke:#334155,stroke-width:1.5px,color:#0f172a
classDef toneBlue fill:#dbeafe,stroke:#2563eb,stroke-width:1.5px,color:#172554
classDef toneAmber fill:#fef3c7,stroke:#d97706,stroke-width:1.5px,color:#78350f
classDef toneMint fill:#dcfce7,stroke:#16a34a,stroke-width:1.5px,color:#14532d
classDef toneRose fill:#ffe4e6,stroke:#e11d48,stroke-width:1.5px,color:#881337
classDef toneIndigo fill:#e0e7ff,stroke:#4f46e5,stroke-width:1.5px,color:#312e81
classDef toneTeal fill:#ccfbf1,stroke:#0f766e,stroke-width:1.5px,color:#134e4a
class node_entry,node_window,node_panels,node_success,node_updateui,node_user toneBlue
class node_app,node_files,node_settings,node_localization,node_avatar_data,node_updates,node_release,node_checksum,node_configdirs,node_local_settings toneAmber
class node_steam,node_transport toneMint
class node_platform_api,node_avatars,node_themes,node_resources toneRose
class node_steamcommunity,node_release_service toneIndigo
```

</details>

## Русский

Программа переносит настройки Dota 2 между Steam-аккаунтами. Скачайте `.exe` для Windows или `.AppImage` для Linux из [релизов](https://github.com/OutTuna/Dota2CFGChanger/releases). Темы и переводы уже внутри.

Закройте Dota 2 и Steam, проверьте пути к `userdata` и запустите сканирование. Слева выберите аккаунт с нужными настройками, справа — тот, куда их перенести. Нажмите кнопку копирования и подтвердите замену.

Копируется папка `<account_id>/570` целиком. Если старые настройки нужны, сохраните их отдельно: после успешной замены временная резервная копия удаляется.

Язык RU / UA / EN и тема выбираются справа сверху и сохраняются автоматически. Dark — тема по умолчанию. Внизу находятся статус, Info и проверка обновлений; Info показывает автора, номер релиза и ссылку GitHub. Имена и аватарки загружаются из Steam Community без входа в аккаунт. При наличии обновления появится окно; файл скачивается только по нажатию. После загрузки закройте старую версию и запустите новую.

## License

[MIT](LICENSE). OutTuna.

## Testers

Thanks for testing builds and helping catch UI issues:

- [Qoudanna](https://github.com/Qoudanna)
- [ciqparis](https://github.com/ciqparis)
- [paradisetears](https://github.com/paradisetears)
