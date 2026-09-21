# Dota 2 Config Manager (Linux)

A native desktop utility for copying Dota 2 client settings from one Steam account to another.

This is the **Linux port**, built from the same source as the [Windows version on `main`](https://github.com/OutTuna/Dota2CFGChanger/tree/main). Feature set and behavior are identical; only the platform-specific bits (folder picker, default Steam path, packaging) differ, as described below.

<p>
  <img alt="Platform: Linux" src="https://img.shields.io/badge/platform-Linux-FCC624?style=flat-square&logo=linux&logoColor=black">
  <img alt="Language: C++17" src="https://img.shields.io/badge/C%2B%2B-17-00599C?style=flat-square">
  <img alt="Build system: CMake" src="https://img.shields.io/badge/build-CMake%203.14%2B-064F8C?style=flat-square">
  <a href="https://github.com/OutTuna/Dota2CFGChanger/releases"><img alt="Releases" src="https://img.shields.io/badge/releases-GitHub-4c566a?style=flat-square"></a>
  <a href="https://github.com/OutTuna/Dota2CFGChanger/commits/linux"><img alt="Last commit" src="https://img.shields.io/github/last-commit/OutTuna/Dota2CFGChanger/linux?style=flat-square"></a>
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

## English

### Overview

Dota 2 keeps keybinds, options and other client-side settings per account, inside Steam's `userdata` directory. This tool copies that one directory (app ID `570`) from one account folder to another, so a setup made once can be reused on a second account, on another machine, or restored from a saved copy.

The program is a single executable with no installer. Third-party libraries are linked statically.

### Features

- Copies only `<account_id>/570`. The rest of the account folder is left alone.
- Scans a source root and a destination root and lists every account folder found in each.
- Shows persona names and avatars, resolved from public Steam Community profiles. Names are cached locally.
- Asks for confirmation before replacing the target's settings.
- Five interface themes, remembered between runs.
- On first launch, tries to guess the Steam `userdata` path itself (native and Flatpak installs).

### Requirements

- Linux with a GPU driver that provides OpenGL (Mesa or proprietary), and an X11 or XWayland session — the app is built against GLFW's X11 backend.
- `zenity` (most GNOME/GTK-based distros already have it) or `kdialog` (KDE/Plasma) for the folder-picker dialog. Without either, the path fields can't be changed through the UI; edit `settings.json` by hand instead (see **Local files** below).
- A Steam `userdata` directory for the destination, and a directory with the same layout for the source (which may be the same directory).

### Usage

1. Download `DotaManager-linux-x86_64.tar.gz` from [Releases](https://github.com/OutTuna/Dota2CFGChanger/releases).
2. Extract it and mark the binary executable:
   ```
   tar xzf DotaManager-linux-x86_64.tar.gz
   cd DotaManager-linux-x86_64
   chmod +x DotaManager
   ```
3. Close Steam, so the client does not write to the directory while it is being replaced.
4. Run `./DotaManager`. On first launch it tries to pre-fill both path fields by checking the common Steam `userdata` locations:
   - `~/.local/share/Steam/userdata` (native package, most common)
   - `~/.steam/steam/userdata` / `~/.steam/root/userdata` (legacy layout / symlink)
   - `~/.var/app/com.valvesoftware.Steam/.local/share/Steam/userdata` (Flatpak)

   If none exist yet, or you want a different location, click the **Откуда конфиг** (source) / **Куда конфиг** (destination) field to open a native folder picker (`zenity`/`kdialog`).
5. Press `SCAN FOLDERS`.
6. Select an account in the left list (source) and one in the right list (destination).
7. Press `COPY CONFIG NOW` and confirm.

Both roots are expected to use Steam's `userdata` layout:

```
<root>/
  <account_id>/
    570/
      ...
```

Only folders whose names consist entirely of digits are treated as accounts.

### Behavior

**Files modified.** The destination's `<account_id>/570` directory is replaced with the source's. This is destructive. The confirmation dialog states that the previous state can only be recovered manually from a `.bak` directory created during the copy, so keep your own backup if the current settings matter.

**Network access.** For every account folder found, the program requests the public profile XML at `steamcommunity.com/profiles/<SteamID64>?xml=1` to read the persona name and the avatar URL, then downloads the avatar. The only data sent is the SteamID64, derived from the folder name as `account_id + 76561197960265728`. No login or API key is used. Private profiles yield no name or avatar, and the list shows the numeric ID instead. Avatar downloads run at most eight at a time.

**Local files.** Written next to the binary (working directory):

| File | Contents |
| --- | --- |
| `settings.json` | source path, destination path, selected theme |
| `nick_cache.json` | account ID to persona name cache |
| `avatar_url_cache.json` | account ID to avatar URL cache |
| `avatar_cache/` | downloaded avatar images, cached on disk |

### Building

Requirements: CMake 3.14 or newer, a C++17 compiler (GCC or Clang), Git, `pkg-config`, and network access during configuration (dependencies are fetched with `FetchContent`).

Debian/Ubuntu package names for the system libraries GLFW/cpr need at build time:

```
sudo apt-get install cmake build-essential pkg-config libgl1-mesa-dev xorg-dev libssl-dev
```

On Arch Linux:

```
sudo pacman -S cmake base-devel pkgconf mesa libx11 libxrandr libxinerama libxcursor libxi openssl
```

Then:

```
git clone -b linux https://github.com/OutTuna/Dota2CFGChanger.git
cd Dota2CFGChanger
cmake -S . -B build
cmake --build build --config Release -j"$(nproc)"
```

The executable is written to `build/DotaManager`. The first configuration also builds cURL and its dependencies through cpr, so it takes noticeably longer than later ones.

Optional install into the system (binary + `.desktop` entry + icon, so the app shows up in your application menu):

```
sudo cmake --install build
```

There's also a helper script that does a full local build/link check the same way CI does (see `scripts/check-full-build-linux.sh --install-deps`), and `scripts/run-all-checks.sh` to run every local check (workflow lint, version-bump unit tests, CMake configure smoke test, and optionally the full build) in one go.

### Dependencies

All dependencies are fetched and built by CMake as static libraries.

| Library | Version | Purpose | License |
| --- | --- | --- | --- |
| [Dear ImGui](https://github.com/ocornut/imgui) | 1.91.6 | User interface | MIT |
| [GLFW](https://github.com/glfw/glfw) | 3.3.8 | Window, input, OpenGL context (X11 backend) | zlib/libpng |
| [cpr](https://github.com/libcpr/cpr) | 1.11.1 | HTTP client (C++ wrapper over libcurl, built against OpenSSL on Linux) | MIT |
| [nlohmann/json](https://github.com/nlohmann/json) | 3.11.3 | Settings and cache files | MIT |
| [stb](https://github.com/nothings/stb) | `master` | Image decoding (`stb_image`) | Public domain / MIT |

### Source layout

| Path | Role |
| --- | --- |
| `main.cpp` | Entry point and main loop |
| `ui.cpp`, `ui.h` | Rendering, themes, dialogs |
| `app_logic.cpp`, `app_logic.h` | Folder scan, config copy, settings, native folder dialog (Windows: `IFileOpenDialog`; Linux: `zenity`/`kdialog`), default Steam path detection on Linux |
| `app_state.h` | Shared application state and file names |
| `avatar.cpp`, `avatar.h` | Asynchronous avatar download and OpenGL texture upload |
| `bg_crimson.h`, `app_icon.h` | Embedded resources (window icon, theme background) |
| `packaging/linux/` | `.desktop` entry and app icon for Linux desktop integration |
| `CMakeLists.txt` | Build definition and dependency pinning |
| `scripts/` | Local CI-equivalent checks (workflow lint, version logic, CMake configure, full Linux build) |

### Limitations

- Requires `zenity` or `kdialog` to change the source/destination paths through the UI. Both are common on desktop distros, but neither is guaranteed to be installed; without one, edit `settings.json` directly instead.
- The Steam installation itself is not detected, only common `userdata` locations are probed as a starting guess (see **Usage**). An unusual custom install path still needs to be picked manually.
- Names and avatars require network access and a public profile.
- Interface strings are currently a mix of English and Russian.

### Issues

Bug reports and feature requests go to the [issue tracker](https://github.com/OutTuna/Dota2CFGChanger/issues). Please include your distro/desktop environment and the release you are running.

---

## Русский

### Описание

Dota 2 хранит раскладку клавиш, опции и другие клиентские настройки отдельно для каждого аккаунта, внутри каталога Steam `userdata`. Утилита копирует один этот каталог (app ID `570`) из папки одного аккаунта в папку другого. Настройки, собранные один раз, можно применить ко второму аккаунту, перенести на другой компьютер или восстановить из сохранённой копии.

Программа поставляется одним исполняемым файлом без установщика. Сторонние библиотеки собраны статически.

Это **Linux-версия**, собранная из того же исходного кода, что и [версия для Windows в ветке `main`](https://github.com/OutTuna/Dota2CFGChanger/tree/main). Функциональность полностью идентична, отличаются только платформенно-зависимые части (диалог выбора папки, путь по умолчанию, упаковка) — они описаны ниже.

### Возможности

- Копируется только `<account_id>/570`. Остальное содержимое папки аккаунта не затрагивается.
- Сканирует исходный и целевой каталоги и показывает найденные в каждом папки аккаунтов.
- Показывает имена и аватары, полученные из публичных профилей Steam Community. Имена кэшируются локально.
- Перед заменой настроек запрашивает подтверждение.
- Пять тем оформления, выбор сохраняется между запусками.
- При первом запуске пытается сам угадать путь к `userdata` Steam (обычная установка и Flatpak).

### Требования

- Linux с видеодрайвером, поддерживающим OpenGL (Mesa или проприетарный), и сессией X11 или XWayland — сборка использует X11-бэкенд GLFW.
- `zenity` (обычно уже есть в дистрибутивах на базе GNOME/GTK) или `kdialog` (KDE/Plasma) для диалога выбора папки. Без них поля путей нельзя изменить через интерфейс — отредактируйте `settings.json` вручную (см. раздел **Локальные файлы** ниже).
- Каталог Steam `userdata` для назначения и каталог с такой же структурой для источника (это может быть один и тот же каталог).

### Использование

1. Скачайте `DotaManager-linux-x86_64.tar.gz` в разделе [Releases](https://github.com/OutTuna/Dota2CFGChanger/releases).
2. Распакуйте и сделайте бинарник исполняемым:
   ```
   tar xzf DotaManager-linux-x86_64.tar.gz
   cd DotaManager-linux-x86_64
   chmod +x DotaManager
   ```
3. Закройте Steam, чтобы клиент не писал в каталог в момент его замены.
4. Запустите `./DotaManager`. При первом запуске программа пытается сама заполнить оба поля пути, проверяя типичные расположения `userdata` Steam:
   - `~/.local/share/Steam/userdata` (обычная установка, самый частый случай)
   - `~/.steam/steam/userdata` / `~/.steam/root/userdata` (старый вариант / симлинк)
   - `~/.var/app/com.valvesoftware.Steam/.local/share/Steam/userdata` (Flatpak)

   Если ни один из путей не найден или нужен другой каталог — нажмите на поле **Откуда конфиг** / **Куда конфиг**, откроется системный диалог выбора папки (`zenity`/`kdialog`).
5. Нажмите `SCAN FOLDERS`.
6. Выберите аккаунт в левом списке (источник) и в правом (назначение).
7. Нажмите `COPY CONFIG NOW` и подтвердите действие.

Оба каталога должны иметь структуру `userdata` из Steam:

```
<root>/
  <account_id>/
    570/
      ...
```

Аккаунтами считаются только папки, имя которых состоит исключительно из цифр.

### Поведение

**Изменяемые файлы.** Каталог `<account_id>/570` в назначении заменяется каталогом из источника. Операция деструктивная. Диалог подтверждения сообщает, что прежнее состояние можно вернуть только вручную из каталога `.bak`, создаваемого на время копирования. Если текущие настройки важны, сделайте собственную резервную копию.

**Сетевые запросы.** Для каждой найденной папки аккаунта программа запрашивает публичный XML профиля по адресу `steamcommunity.com/profiles/<SteamID64>?xml=1`, берёт из него имя и ссылку на аватар, затем загружает аватар. Передаётся только SteamID64, вычисленный из имени папки как `account_id + 76561197960265728`. Логин и API-ключ не используются. Для закрытых профилей имя и аватар недоступны, и в списке показывается числовой ID. Одновременно загружается не более восьми аватаров.

**Локальные файлы.** Записываются рядом с бинарником (в рабочем каталоге):

| Файл | Содержимое |
| --- | --- |
| `settings.json` | путь к источнику, путь к назначению, выбранная тема |
| `nick_cache.json` | кэш соответствия ID аккаунта и имени |
| `avatar_url_cache.json` | кэш ссылок на аватары по ID аккаунта |
| `avatar_cache/` | скачанные аватары, кэшированные на диске |

### Сборка

Требования: CMake 3.14 или новее, компилятор C++17 (GCC или Clang), Git, `pkg-config`, доступ к сети на этапе конфигурации (зависимости загружаются через `FetchContent`).

Пакеты для Debian/Ubuntu (системные библиотеки, нужные GLFW/cpr на этапе сборки):

```
sudo apt-get install cmake build-essential pkg-config libgl1-mesa-dev xorg-dev libssl-dev
```

Для Arch Linux:

```
sudo pacman -S cmake base-devel pkgconf mesa libx11 libxrandr libxinerama libxcursor libxi openssl
```

Далее:

```
git clone -b linux https://github.com/OutTuna/Dota2CFGChanger.git
cd Dota2CFGChanger
cmake -S . -B build
cmake --build build --config Release -j"$(nproc)"
```

Исполняемый файл создаётся по пути `build/DotaManager`. Первая конфигурация дополнительно собирает cURL и его зависимости через cpr, поэтому занимает заметно больше времени, чем последующие.

Опциональная установка в систему (бинарник + `.desktop`-файл + иконка, чтобы программа появилась в меню приложений):

```
sudo cmake --install build
```

Есть вспомогательный скрипт, который делает полную локальную проверку сборки так же, как CI (`scripts/check-full-build-linux.sh --install-deps`), и `scripts/run-all-checks.sh`, запускающий разом все локальные проверки (линт workflow'ов, юнит-тесты логики версий, smoke-тест конфигурации CMake и опционально полную сборку).

### Зависимости

Все зависимости загружаются и собираются CMake как статические библиотеки.

| Библиотека | Версия | Назначение | Лицензия |
| --- | --- | --- | --- |
| [Dear ImGui](https://github.com/ocornut/imgui) | 1.91.6 | Пользовательский интерфейс | MIT |
| [GLFW](https://github.com/glfw/glfw) | 3.3.8 | Окно, ввод, контекст OpenGL (X11-бэкенд) | zlib/libpng |
| [cpr](https://github.com/libcpr/cpr) | 1.11.1 | HTTP-клиент (обёртка над libcurl, на Linux собирается с OpenSSL) | MIT |
| [nlohmann/json](https://github.com/nlohmann/json) | 3.11.3 | Файлы настроек и кэша | MIT |
| [stb](https://github.com/nothings/stb) | `master` | Декодирование изображений (`stb_image`) | Public domain / MIT |

### Структура исходников

| Путь | Назначение |
| --- | --- |
| `main.cpp` | Точка входа и главный цикл |
| `ui.cpp`, `ui.h` | Отрисовка, темы, диалоги |
| `app_logic.cpp`, `app_logic.h` | Сканирование папок, копирование, настройки, системный диалог выбора папки (Windows: `IFileOpenDialog`; Linux: `zenity`/`kdialog`), определение пути Steam по умолчанию на Linux |
| `app_state.h` | Общее состояние приложения и имена файлов |
| `avatar.cpp`, `avatar.h` | Асинхронная загрузка аватаров и создание текстур OpenGL |
| `bg_crimson.h`, `app_icon.h` | Встроенные ресурсы (иконка окна, фон темы) |
| `packaging/linux/` | `.desktop`-файл и иконка для интеграции с рабочим столом Linux |
| `CMakeLists.txt` | Описание сборки и версии зависимостей |
| `scripts/` | Локальные проверки, эквивалентные CI (линт workflow'ов, логика версий, конфигурация CMake, полная сборка под Linux) |

### Ограничения

- Для изменения путей источника/назначения через интерфейс нужен `zenity` или `kdialog`. Оба часто уже установлены в десктопных дистрибутивах, но это не гарантировано — без них правьте `settings.json` вручную.
- Сама установка Steam не определяется — проверяются только типичные расположения `userdata` как стартовая догадка (см. **Использование**). Нестандартный путь установки всё равно нужно выбрать вручную.
- Для имён и аватаров нужны сеть и публичный профиль.
- Строки интерфейса сейчас частично на английском, частично на русском.

### Обратная связь

Сообщения об ошибках и предложения принимаются в [трекере задач](https://github.com/OutTuna/Dota2CFGChanger/issues). Укажите ваш дистрибутив/окружение рабочего стола и используемый релиз.
