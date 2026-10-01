# Dota 2 Config Manager

A native Windows and Linux utility for copying Dota 2 client settings from one Steam account to another.

<p>
  <img alt="Platform: Windows & Linux" src="https://img.shields.io/badge/platform-Windows%20%7C%20Linux-0078D6?style=flat-square">
  <img alt="Build: Windows + Linux" src="https://img.shields.io/github/actions/workflow/status/OutTuna/Dota2CFGChanger/build.yaml?style=flat-square">
  <img alt="Language: C++17" src="https://img.shields.io/badge/C%2B%2B-17-00599C?style=flat-square">
  <img alt="Build system: CMake" src="https://img.shields.io/badge/build-CMake%203.14%2B-064F8C?style=flat-square">
  <a href="https://github.com/OutTuna/Dota2CFGChanger/releases"><img alt="Releases" src="https://img.shields.io/badge/releases-GitHub-4c566a?style=flat-square"></a>
  <a href="LICENSE"><img alt="License: MIT" src="https://img.shields.io/github/license/OutTuna/Dota2CFGChanger?style=flat-square"></a>
  <a href="https://github.com/OutTuna/Dota2CFGChanger/commits/main"><img alt="Last commit" src="https://img.shields.io/github/last-commit/OutTuna/Dota2CFGChanger?style=flat-square"></a>
</p>

<p>
  <a href="https://github.com/ocornut/imgui"><img alt="Dear ImGui 1.91.6" src="https://img.shields.io/badge/Dear_ImGui-1.91.6-4c566a?style=flat-square"></a>
  <a href="https://github.com/glfw/glfw"><img alt="GLFW 3.3.8" src="https://img.shields.io/badge/GLFW-3.3.8-4c566a?style=flat-square"></a>
  <img alt="OpenGL 3 backend" src="https://img.shields.io/badge/OpenGL-3%20backend-4c566a?style=flat-square">
  <a href="https://github.com/libcpr/cpr"><img alt="cpr 1.11.1" src="https://img.shields.io/badge/cpr-1.11.1-4c566a?style=flat-square"></a>
  <a href="https://github.com/nlohmann/json"><img alt="nlohmann/json 3.11.3" src="https://img.shields.io/badge/nlohmann%2Fjson-3.11.3-4c566a?style=flat-square"></a>
  <a href="https://github.com/nothings/stb"><img alt="stb_image" src="https://img.shields.io/badge/stb__image-single--header-4c566a?style=flat-square"></a>
</p>

[English](#english) | [Русский](#русский)

---

## Digram of project

<img width="5528" height="8809" alt="diagram_new" src="https://github.com/OutTuna/Dota2CFGChanger/blob/main/diagram_new.png" />


## English

### Overview

Dota 2 keeps keybinds, options and other client-side settings per account, inside Steam's `userdata` directory. This tool copies that one directory (app ID `570`) from one account folder to another, so a setup made once can be reused on a second account, on another machine, or restored from a saved copy.

The program is a single executable with no installer: a portable `.exe` on Windows and an AppImage on Linux. Third-party libraries are linked statically.

### Features

- Copies only `<account_id>/570`. The rest of the account folder is left alone.
- Scans a source root and a destination root and lists every account folder found in each.
- Shows persona names and avatars, resolved from public Steam Community profiles. Names and avatars are cached locally, on disk.
- Asks for confirmation before replacing the target's settings.
- Five interface themes, remembered between runs. All theme JSON files and the Crimson background are embedded in the executable. An optional `themes/` folder next to it can override the embedded resources; missing or invalid files fall back to the embedded versions. The Crimson theme adds a live palette editor and a custom background.

### Requirements

- Windows with a GPU driver that provides OpenGL, or a Linux desktop with OpenGL (mesa or vendor driver).
- On Linux: `zenity` or `kdialog` for the folder picker, and a system font with Cyrillic coverage (DejaVu, Liberation or Noto — any desktop distro ships one).
- A Steam `userdata` directory for the destination, and a directory with the same layout for the source (which may be the same directory).

### Usage

1. Download `DotaManager.exe` (Windows) or the AppImage (Linux) from [Releases](https://github.com/OutTuna/Dota2CFGChanger/releases).
2. Close Steam, so the client does not write to the directory while it is being replaced.
3. Run the program. Click **Source config folder** to choose the source root and **Destination account folder** to choose the target root. Both default to the detected Steam userdata directory, or a standard install location if none is found.
4. Press `Scan folders`.
5. Select an account in the left list (source) and one in the right list (destination).
6. Press `Copy config` and confirm.

Both roots are expected to use Steam's `userdata` layout:

```
<root>/
  <account_id>/
    570/
      ...
```

Only folders whose names consist entirely of digits are treated as accounts.

### Behavior

**Files modified.** The destination's `<account_id>/570` directory is replaced with the source's. The copy is written to a sibling `.dotamanager_tmp` folder first. The old config is renamed to `.dotamanager_backup` before installing the new one; if installation fails, the program tries to restore the old directory. If restoration fails, both working directories are preserved and the backup path is shown. Leftovers from an interrupted copy block another copy until you preserve or restore them manually. Once the copy succeeds, however, the operation is destructive: the temporary backup is removed and there is no permanent automatic backup, so keep your own backup if the current settings matter.

**Network access.** For every account folder found, the program requests the public profile XML at `steamcommunity.com/profiles/<SteamID64>?xml=1` to read the persona name and the avatar URL, then downloads the avatar. The only data sent is the SteamID64, derived from the folder name as `account_id + 76561197960265728`. No login or API key is used. Private profiles yield no name or avatar, and the list shows the numeric ID instead. At most eight avatars download at a time; a failed download is retried after 30 seconds.

**Local files.** Settings and caches are written to a per-user config directory — `%APPDATA%\DotaManager` on Windows, `$XDG_CONFIG_HOME/DotaManager` (or `~/.config/DotaManager`) on Linux — created on first run, not next to the executable:

| File / folder | Contents |
| --- | --- |
| `settings.json` | source path, destination path, selected theme and language |
| `nick_cache.json` | account ID to persona name cache |
| `avatar_url_cache.json` | account ID to avatar URL cache |
| `avatar_cache/` | downloaded avatar images (`<account_id>.img`) |

### Building

Requirements: CMake 3.14 or newer, a C++17 compiler (MSVC or MinGW on Windows, GCC/Clang on Linux), Git, and network access during configuration, because dependencies are fetched with `FetchContent`.

```
git clone https://github.com/OutTuna/Dota2CFGChanger.git
cd Dota2CFGChanger
cmake -S . -B build
cmake --build build --config Release
```

- Windows (Visual Studio generator): the executable is written to `build/Release/DotaManager.exe`.
- Linux: install the dev packages first (`build-essential cmake libgl1-mesa-dev libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libssl-dev`), configure with `-DCMAKE_BUILD_TYPE=Release`; the executable is `build/DotaManager`.

The first configuration also builds cURL and its dependencies through cpr, so it takes noticeably longer than later ones. An `-DAPP_VERSION=<x.y>` option stamps the Windows version resource; CI computes it automatically.

### Dependencies

All dependencies are fetched and built by CMake as static libraries.

| Library | Version | Purpose | License |
| --- | --- | --- | --- |
| [Dear ImGui](https://github.com/ocornut/imgui) | 1.91.6 | User interface | MIT |
| [GLFW](https://github.com/glfw/glfw) | 3.3.8 | Window, input, OpenGL context | zlib/libpng |
| [cpr](https://github.com/libcpr/cpr) | 1.11.1 | HTTP client (C++ wrapper over libcurl) | MIT |
| [nlohmann/json](https://github.com/nlohmann/json) | 3.11.3 | Settings and cache files | MIT |
| [stb](https://github.com/nothings/stb) | pinned commit | Image decoding (`stb_image`) | Public domain / MIT |

### Source layout

| Path | Role |
| --- | --- |
| `main.cpp` | Entry point and main loop |
| `app.cpp`, `app.h` | Scan/copy coordination, settings and caches, managed scan worker |
| `steam_api.cpp`, `steam_api.h` | Steam profile requests and XML parsing |
| `platform.cpp`, `platform.h` | Native paths, folder dialogs, fonts and external links |
| `ui/` | Rendering, JSON theme system, dialogs, popups |
| `file_ops.cpp`, `file_ops.h` | Path normalization, Steam detection, recoverable config replacement |
| `localization.cpp`, `localization.h`, `locales/` | Embedded English, Russian and Ukrainian translations |
| `avatar_data.cpp`, `avatar_data.h` | Background avatar downloads, disk cache and image decoding |
| `avatar.cpp`, `avatar.h` | OpenGL texture upload and rendering adapter |
| `updates.*`, `update_transport.*`, `release_info.*`, `checksum.*` | Update state, GitHub requests, version comparison and SHA-256 validation |
| `app_icon.h`, `cmake/` | Embedded icon and resource generators |
| `app.rc.in`, `app.manifest`, `Icon.ico` | Windows version info, manifest and icon (assembled by CMake) |
| `themes/` | Theme JSON files and background, embedded at build time |
| `scripts/` | Local development checks (`run-all-checks.sh`, versioning, CMake smoke tests) |
| `.github/workflows/build.yaml` | CI: builds Windows `.exe` and Linux AppImage, publishes a release |
| `CMakeLists.txt` | Build definition and dependency pinning |

### Limitations

- No macOS support.
- Steam userdata is detected from the Windows registry and common Linux locations, including Flatpak and `$XDG_DATA_HOME`. When several installations exist, the first detected directory is selected; choose another manually if needed.
- On Linux the folder picker requires `zenity` or `kdialog`.
- Names and avatars require network access and a public profile.
- English is the default interface language. Russian and Ukrainian are available in settings, and the selected language is remembered.

### Issues

Bug reports and feature requests go to the [issue tracker](https://github.com/OutTuna/Dota2CFGChanger/issues). Please include your OS version and the release you are running.

---

## Русский

### Описание

Dota 2 хранит раскладку клавиш, опции и другие клиентские настройки отдельно для каждого аккаунта, внутри каталога Steam `userdata`. Утилита копирует один этот каталог (app ID `570`) из папки одного аккаунта в папку другого. Настройки, собранные один раз, можно применить ко второму аккаунту, перенести на другой компьютер или восстановить из сохранённой копии.

Программа поставляется одним исполняемым файлом без установщика: портативный `.exe` на Windows и AppImage на Linux. Сторонние библиотеки собраны статически.

### Возможности

- Копируется только `<account_id>/570`. Остальное содержимое папки аккаунта не затрагивается.
- Сканирует исходный и целевой каталоги и показывает найденные в каждом папки аккаунтов.
- Показывает имена и аватары, полученные из публичных профилей Steam Community. Имена и аватары кэшируются локально, на диске.
- Перед заменой настроек запрашивает подтверждение.
- Пять тем оформления, выбор сохраняется между запусками. Все JSON-темы и фон Crimson встроены в исполняемый файл. Необязательная папка `themes/` рядом с ним позволяет переопределить ресурсы; при отсутствии или повреждении файлов используются встроенные версии. Тема Crimson добавляет редактор палитры и собственный фон.

### Требования

- Windows с видеодрайвером с поддержкой OpenGL либо Linux-десктоп с OpenGL (mesa или драйвер вендора).
- На Linux: `zenity` или `kdialog` для диалога выбора папки и системный шрифт с кириллицей (DejaVu, Liberation или Noto — есть в любом десктопном дистрибутиве).
- Каталог Steam `userdata` для назначения и каталог с такой же структурой для источника (это может быть один и тот же каталог).

### Использование

1. Скачайте `DotaManager.exe` (Windows) или AppImage (Linux) в разделе [Releases](https://github.com/OutTuna/Dota2CFGChanger/releases).
2. Закройте Steam, чтобы клиент не писал в каталог в момент его замены.
3. Запустите программу. Нажмите **Source config folder** для выбора источника и **Destination account folder** для назначения. Оба пути по умолчанию указывают на найденный каталог Steam userdata либо стандартный путь установки. В настройках можно выбрать русский или украинский язык.
4. Нажмите `Scan folders`.
5. Выберите аккаунт в левом списке (источник) и в правом (назначение).
6. Нажмите `Copy config` и подтвердите действие.

Оба каталога должны иметь структуру `userdata` из Steam:

```
<root>/
  <account_id>/
    570/
      ...
```

Аккаунтами считаются только папки, имя которых состоит исключительно из цифр.

### Поведение

**Изменяемые файлы.** Каталог `<account_id>/570` в назначении заменяется каталогом из источника. Сначала копия записывается в соседнюю папку `.dotamanager_tmp`. Старый конфиг переименовывается в `.dotamanager_backup` перед установкой нового; при ошибке программа пытается вернуть его обратно. Если восстановление не удалось, обе рабочие папки сохраняются, а путь к резервной копии показывается в статусе. Остатки прерванного копирования блокируют повторную попытку до их ручного сохранения или восстановления. После успешного копирования операция деструктивна: временная резервная копия удаляется, постоянной автоматической копии нет, поэтому, если текущие настройки важны, сделайте собственную.

**Сетевые запросы.** Для каждой найденной папки аккаунта программа запрашивает публичный XML профиля по адресу `steamcommunity.com/profiles/<SteamID64>?xml=1`, берёт из него имя и ссылку на аватар, затем загружает аватар. Передаётся только SteamID64, вычисленный из имени папки как `account_id + 76561197960265728`. Логин и API-ключ не используются. Для закрытых профилей имя и аватар недоступны, и в списке показывается числовой ID. Одновременно загружается не более восьми аватаров; неудачная загрузка повторяется через 30 секунд.

**Локальные файлы.** Настройки и кэши записываются в пользовательский каталог конфигурации — `%APPDATA%\DotaManager` на Windows, `$XDG_CONFIG_HOME/DotaManager` (или `~/.config/DotaManager`) на Linux — он создаётся при первом запуске, а не рядом с исполняемым файлом:

| Файл / папка | Содержимое |
| --- | --- |
| `settings.json` | путь к источнику, путь к назначению, выбранные тема и язык |
| `nick_cache.json` | кэш соответствия ID аккаунта и имени |
| `avatar_url_cache.json` | кэш соответствия ID аккаунта и ссылки на аватар |
| `avatar_cache/` | скачанные изображения аватаров (`<account_id>.img`) |

### Сборка

Требования: CMake 3.14 или новее, компилятор C++17 (MSVC или MinGW на Windows, GCC/Clang на Linux), Git и доступ к сети на этапе конфигурации, поскольку зависимости загружаются через `FetchContent`.

```
git clone https://github.com/OutTuna/Dota2CFGChanger.git
cd Dota2CFGChanger
cmake -S . -B build
cmake --build build --config Release
```

- Windows (генератор Visual Studio): исполняемый файл создаётся по пути `build/Release/DotaManager.exe`.
- Linux: сначала установите dev-пакеты (`build-essential cmake libgl1-mesa-dev libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libssl-dev`), конфигурируйте с `-DCMAKE_BUILD_TYPE=Release`; исполняемый файл — `build/DotaManager`.

Первая конфигурация дополнительно собирает cURL и его зависимости через cpr, поэтому занимает заметно больше времени, чем последующие. Опция `-DAPP_VERSION=<x.y>` записывает версию в ресурс Windows-файла; в CI версия вычисляется автоматически.

### Зависимости

Все зависимости загружаются и собираются CMake как статические библиотеки.

| Библиотека | Версия | Назначение | Лицензия |
| --- | --- | --- | --- |
| [Dear ImGui](https://github.com/ocornut/imgui) | 1.91.6 | Пользовательский интерфейс | MIT |
| [GLFW](https://github.com/glfw/glfw) | 3.3.8 | Окно, ввод, контекст OpenGL | zlib/libpng |
| [cpr](https://github.com/libcpr/cpr) | 1.11.1 | HTTP-клиент (обёртка над libcurl) | MIT |
| [nlohmann/json](https://github.com/nlohmann/json) | 3.11.3 | Файлы настроек и кэша | MIT |
| [stb](https://github.com/nothings/stb) | зафиксированный коммит | Декодирование изображений (`stb_image`) | Public domain / MIT |

### Структура исходников

| Путь | Назначение |
| --- | --- |
| `main.cpp` | Точка входа и главный цикл |
| `app.cpp`, `app.h` | Управление сканированием и копированием, настройки и кэши |
| `steam_api.cpp`, `steam_api.h` | Запросы к Steam и разбор XML |
| `platform.cpp`, `platform.h` | Пути, диалоги выбора папки, шрифты и внешние ссылки |
| `ui/` | Отрисовка, JSON-система тем, диалоги, всплывающие окна |
| `file_ops.cpp`, `file_ops.h` | Пути, обнаружение Steam, замена конфига с восстановлением |
| `localization.cpp`, `localization.h`, `locales/` | Встроенные английские, русские и украинские переводы |
| `avatar_data.cpp`, `avatar_data.h` | Загрузка аватаров, дисковый кэш и декодирование изображений |
| `avatar.cpp`, `avatar.h` | Создание текстур OpenGL и адаптер отрисовки |
| `updates.*`, `update_transport.*`, `release_info.*`, `checksum.*` | Проверка и загрузка обновлений, сравнение версий, SHA-256 |
| `app_icon.h`, `cmake/` | Встроенная иконка и генераторы ресурсов |
| `app.rc.in`, `app.manifest`, `Icon.ico` | Информация о версии, манифест и иконка Windows (собираются CMake) |
| `themes/` | JSON-файлы тем и фон, встраиваются при сборке |
| `scripts/` | Локальные проверки для разработки (`run-all-checks.sh`, версии, smoke-тесты CMake) |
| `.github/workflows/build.yaml` | CI: собирает `.exe` для Windows и AppImage для Linux, публикует релиз |
| `CMakeLists.txt` | Описание сборки и версии зависимостей |

### Ограничения

- Нет поддержки macOS.
- Каталог Steam userdata определяется по реестру Windows и стандартным Linux-путям, включая Flatpak и `$XDG_DATA_HOME`. Если установок несколько, выбирается первый найденный каталог; другой можно выбрать вручную.
- На Linux диалогу выбора папки нужны `zenity` или `kdialog`.
- Для имён и аватаров нужны сеть и публичный профиль.
- По умолчанию интерфейс на английском. Русский и украинский доступны в настройках; выбранный язык сохраняется.

### Обратная связь

Сообщения об ошибках и предложения принимаются в [трекере задач](https://github.com/OutTuna/Dota2CFGChanger/issues). Укажите версию ОС и используемый релиз.


Core regression checks (paths, copy failures, rollback and translations):

```bash
python3 tests/core_checks.py build/_deps/json-src/include
```

These checks need Python 3 and a C++17 compiler and run in a temporary directory on Linux/macOS.

### Updates and author link

The footer's **OutTuna** button opens the repository. **Check updates** opens the update dialog. Release builds also check once at startup in the background; a newer version opens the dialog, while an offline startup stays quiet. Development builds with version `0.0` only check manually.

The checker uses the repository's `latest` release tag, including prereleases, and reads the numeric version from `Latest Build (vX.Y)`. No GitHub login is required. This adds a request to `api.github.com` at startup and when checking manually.

**Download** fetches the Windows `.exe` or Linux x86-64 `.AppImage` only after a click. The file is streamed into `<config directory>/updates/<version>/`, verified against GitHub's asset size and SHA-256 digest, then renamed from `.part`. The dialog can open that folder. Close the old application and run the downloaded file; no running executable is overwritten. If a matching asset or digest is unavailable, **Open GitHub release** provides manual downloading. Interrupted or invalid partial downloads are removed.

На панели снизу **OutTuna** открывает репозиторий, а **Проверить обновления** — окно обновления. Релизная сборка также проверяет обновления в фоне при запуске; окно появляется при наличии новой версии. Ошибка фоновой проверки не мешает работе приложения. Версия `0.0` проверяется только вручную.

**Скачать** загружает файл только после нажатия, в `<каталог настроек>/updates/<версия>/`. Размер и SHA-256 проверяются до завершения загрузки. Затем можно открыть папку, закрыть старую программу и запустить новый файл. Текущий исполняемый файл не перезаписывается. Кнопка **Открыть релиз GitHub** позволяет скачать вручную.

### CI cache and regression checks

CI caches `build/_deps` separately by OS, architecture, compiler identity and `CMakeLists.txt`. Linux also keeps a 500 MB compiler cache through ccache. Every run configures and builds the application; a cache hit never substitutes an old application executable. The first run fills the cache, so compare later runs to measure the speedup.

`dotamanager_core` contains file operations, Steam/avatars, settings, translations and updates without ImGui/OpenGL dependencies. The GUI owns texture upload and window rendering.

```bash
cmake -S . -B build -DDOTAMANAGER_BUILD_TESTS=ON
cmake --build build --config Release
ctest --test-dir build --build-config Release --output-on-failure
python3 tests/core_checks.py build/_deps/json-src/include
```

CI runs update metadata/checksum tests, update cancellation/download tests headless avatar-cache tests and update-dialog layout checks in all three languages on Windows and Linux. Linux also runs the path, rollback and localization checks.
