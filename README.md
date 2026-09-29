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
  <a href="https://gitdiagram.com/outtuna/dota2cfgchanger?utm_source=readme&utm_medium=badge"><img alt="Diagram" src="https://gitdiagram.com/diagram-badge.svg"></a>
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

<img width="4895" height="6747" alt="diagram" src="https://github.com/user-attachments/assets/40e7e300-9a03-4a88-a3a1-0c60b29f2223" />


## English

### Overview

Dota 2 keeps keybinds, options and other client-side settings per account, inside Steam's `userdata` directory. This tool copies that one directory (app ID `570`) from one account folder to another, so a setup made once can be reused on a second account, on another machine, or restored from a saved copy.

The program is a single executable with no installer: a portable `.exe` on Windows and an AppImage on Linux. Third-party libraries are linked statically.

### Features

- Copies only `<account_id>/570`. The rest of the account folder is left alone.
- Scans a source root and a destination root and lists every account folder found in each.
- Shows persona names and avatars, resolved from public Steam Community profiles. Names and avatars are cached locally, on disk.
- Asks for confirmation before replacing the target's settings.
- Five interface themes, remembered between runs. Themes are defined by JSON files in a `themes/` folder next to the executable; if that folder is missing, built-in fallbacks are used. The Crimson theme adds a live palette editor and a custom background.

### Requirements

- Windows with a GPU driver that provides OpenGL, or a Linux desktop with OpenGL (mesa or vendor driver).
- On Linux: `zenity` or `kdialog` for the folder picker, and a system font with Cyrillic coverage (DejaVu, Liberation or Noto — any desktop distro ships one).
- A Steam `userdata` directory for the destination, and a directory with the same layout for the source (which may be the same directory).

### Usage

1. Download `DotaManager.exe` (Windows) or the AppImage (Linux) from [Releases](https://github.com/OutTuna/Dota2CFGChanger/releases).
2. Close Steam, so the client does not write to the directory while it is being replaced.
3. Run the program. Click the **Откуда конфиг** (source) field and choose the source root. The destination defaults to `C:\Program Files (x86)\Steam\userdata` on Windows and `~/.local/share/Steam/userdata` on Linux; click its field to change it.
4. Press `SCAN FOLDERS`.
5. Select an account in the left list (source) and one in the right list (destination).
6. Press `COPY CONFIG NOW` and confirm.

Both roots are expected to use Steam's `userdata` layout:

```
<root>/
  <account_id>/
    570/
      ...
```

Only folders whose names consist entirely of digits are treated as accounts.

### Behavior

**Files modified.** The destination's `<account_id>/570` directory is replaced with the source's. The copy is written to a temporary folder first and swapped in only when it completes, so a failed copy (disk full, file locked by Dota) leaves the existing config untouched. Once the copy succeeds, however, the operation is destructive: the previous config is gone and there is no automatic backup, so keep your own backup if the current settings matter.

**Network access.** For every account folder found, the program requests the public profile XML at `steamcommunity.com/profiles/<SteamID64>?xml=1` to read the persona name and the avatar URL, then downloads the avatar. The only data sent is the SteamID64, derived from the folder name as `account_id + 76561197960265728`. No login or API key is used. Private profiles yield no name or avatar, and the list shows the numeric ID instead. At most eight avatars download at a time; a failed download is retried after 30 seconds.

**Local files.** Settings and caches are written to a per-user config directory — `%APPDATA%\DotaManager` on Windows, `$XDG_CONFIG_HOME/DotaManager` (or `~/.config/DotaManager`) on Linux — created on first run, not next to the executable:

| File / folder | Contents |
| --- | --- |
| `settings.json` | source path, destination path, selected theme |
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

- Windows (Visual Studio generator): the executable is written to `build/Release/DotaManager.exe`, and a `themes/` folder is copied next to it.
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
| `app.cpp`, `app.h` | Folder scan, config copy, settings and caches, Steam profile requests, folder dialog, font lookup |
| `ui.cpp`, `ui.h` | Rendering, JSON theme system, dialogs, popups |
| `avatar.cpp`, `avatar.h` | Asynchronous avatar download, disk cache and OpenGL texture upload |
| `app_icon.h`, `bg_crimson.h` | Embedded icon and fallback background image |
| `app.rc.in`, `app.manifest`, `Icon.ico` | Windows version info, manifest and icon (assembled by CMake) |
| `themes/` | Theme JSON files, copied next to the executable at build time |
| `scripts/` | Local development checks (`run-all-checks.sh`, versioning, CMake smoke tests) |
| `.github/workflows/build.yaml` | CI: builds Windows `.exe` and Linux AppImage, publishes a release |
| `CMakeLists.txt` | Build definition and dependency pinning |

### Limitations

- No macOS support.
- The Steam installation is not detected. The defaults are the standard install paths (`C:\Program Files (x86)\Steam\userdata`, `~/.local/share/Steam/userdata`); Flatpak Steam lives elsewhere and must be browsed to manually.
- On Linux the folder picker requires `zenity` or `kdialog`.
- Names and avatars require network access and a public profile.
- Interface strings are currently a mix of English and Russian.

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
- Пять тем оформления, выбор сохраняется между запусками. Темы описываются JSON-файлами в папке `themes/` рядом с исполняемым файлом; если папки нет, используются встроенные резервные варианты. Тема Crimson добавляет редактор палитры и собственный фон.

### Требования

- Windows с видеодрайвером с поддержкой OpenGL либо Linux-десктоп с OpenGL (mesa или драйвер вендора).
- На Linux: `zenity` или `kdialog` для диалога выбора папки и системный шрифт с кириллицей (DejaVu, Liberation или Noto — есть в любом десктопном дистрибутиве).
- Каталог Steam `userdata` для назначения и каталог с такой же структурой для источника (это может быть один и тот же каталог).

### Использование

1. Скачайте `DotaManager.exe` (Windows) или AppImage (Linux) в разделе [Releases](https://github.com/OutTuna/Dota2CFGChanger/releases).
2. Закройте Steam, чтобы клиент не писал в каталог в момент его замены.
3. Запустите программу. Нажмите на поле **Откуда конфиг** и выберите исходный каталог. Целевой каталог по умолчанию: `C:\Program Files (x86)\Steam\userdata` на Windows и `~/.local/share/Steam\userdata` на Linux. Чтобы изменить его, нажмите на соответствующее поле.
4. Нажмите `SCAN FOLDERS`.
5. Выберите аккаунт в левом списке (источник) и в правом (назначение).
6. Нажмите `COPY CONFIG NOW` и подтвердите действие.

Оба каталога должны иметь структуру `userdata` из Steam:

```
<root>/
  <account_id>/
    570/
      ...
```

Аккаунтами считаются только папки, имя которых состоит исключительно из цифр.

### Поведение

**Изменяемые файлы.** Каталог `<account_id>/570` в назначении заменяется каталогом из источника. Сначала копия пишется во временную папку и подменяется только после успешного завершения, поэтому неудачное копирование (нет места на диске, файл занят Dota) не трогает существующий конфиг. После успешного копирования операция деструктивна: прежний конфиг удаётся без автоматической резервной копии, поэтому, если текущие настройки важны, сделайте собственную.

**Сетевые запросы.** Для каждой найденной папки аккаунта программа запрашивает публичный XML профиля по адресу `steamcommunity.com/profiles/<SteamID64>?xml=1`, берёт из него имя и ссылку на аватар, затем загружает аватар. Передаётся только SteamID64, вычисленный из имени папки как `account_id + 76561197960265728`. Логин и API-ключ не используются. Для закрытых профилей имя и аватар недоступны, и в списке показывается числовой ID. Одновременно загружается не более восьми аватаров; неудачная загрузка повторяется через 30 секунд.

**Локальные файлы.** Настройки и кэши записываются в пользовательский каталог конфигурации — `%APPDATA%\DotaManager` на Windows, `$XDG_CONFIG_HOME/DotaManager` (или `~/.config/DotaManager`) на Linux — он создаётся при первом запуске, а не рядом с исполняемым файлом:

| Файл / папка | Содержимое |
| --- | --- |
| `settings.json` | путь к источнику, путь к назначению, выбранная тема |
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

- Windows (генератор Visual Studio): исполняемый файл создаётся по пути `build/Release/DotaManager.exe`, рядом копируется папка `themes/`.
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
| `app.cpp`, `app.h` | Сканирование папок, копирование, настройки и кэши, запросы к профилям Steam, диалог выбора папки, поиск шрифтов |
| `ui.cpp`, `ui.h` | Отрисовка, JSON-система тем, диалоги, всплывающие окна |
| `avatar.cpp`, `avatar.h` | Асинхронная загрузка аватаров, дисковый кэш и создание текстур OpenGL |
| `app_icon.h`, `bg_crimson.h` | Встроенная иконка и резервное фоновое изображение |
| `app.rc.in`, `app.manifest`, `Icon.ico` | Информация о версии, манифест и иконка Windows (собираются CMake) |
| `themes/` | JSON-файлы тем, копируются при сборке рядом с исполняемым файлом |
| `scripts/` | Локальные проверки для разработки (`run-all-checks.sh`, версии, smoke-тесты CMake) |
| `.github/workflows/build.yaml` | CI: собирает `.exe` для Windows и AppImage для Linux, публикует релиз |
| `CMakeLists.txt` | Описание сборки и версии зависимостей |

### Ограничения

- Нет поддержки macOS.
- Каталог установки Steam не определяется автоматически. По умолчанию используются стандартные пути (`C:\Program Files (x86)\Steam\userdata`, `~/.local/share/Steam\userdata`); Steam из Flatpak лежит в другом месте, до него нужно дойти вручную.
- На Linux диалогу выбора папки нужны `zenity` или `kdialog`.
- Для имён и аватаров нужны сеть и публичный профиль.
- Строки интерфейса сейчас частично на английском, частично на русском.

### Обратная связь

Сообщения об ошибках и предложения принимаются в [трекере задач](https://github.com/OutTuna/Dota2CFGChanger/issues). Укажите версию ОС и используемый релиз.
