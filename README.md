# Dota 2 Config Manager

A native Windows utility for copying Dota 2 client settings from one Steam account to another.

<p>
  <img alt="Platform: Windows" src="https://img.shields.io/badge/platform-Windows-0078D6?style=flat-square">
  <img alt="Build: Windows" src="https://img.shields.io/github/actions/workflow/status/OutTuna/Dota2CFGChanger/build.yaml?style=flat-square">
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

### Requirements

- Windows with a GPU driver that provides OpenGL.
- A Steam `userdata` directory for the destination, and a directory with the same layout for the source (which may be the same directory).

### Usage

1. Download `DotaManager.exe` from [Releases](https://github.com/OutTuna/Dota2CFGChanger/releases).
2. Close Steam, so the client does not write to the directory while it is being replaced.
3. Run the program. Click the **Откуда конфиг** (source) field and choose the source root. The destination defaults to `C:\Program Files (x86)\Steam\userdata`; click its field to change it.
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

**Files modified.** The destination's `<account_id>/570` directory is replaced with the source's. This is destructive. The confirmation dialog states that the previous state can only be recovered manually from a `.bak` directory created during the copy, so keep your own backup if the current settings matter.

**Network access.** For every account folder found, the program requests the public profile XML at `steamcommunity.com/profiles/<SteamID64>?xml=1` to read the persona name and the avatar URL, then downloads the avatar. The only data sent is the SteamID64, derived from the folder name as `account_id + 76561197960265728`. No login or API key is used. Private profiles yield no name or avatar, and the list shows the numeric ID instead. Avatar downloads run at most four at a time.

**Local files.** Two JSON files are written to the working directory:

| File | Contents |
| --- | --- |
| `settings.json` | source path, destination path, selected theme |
| `nicknames.json` | account ID to persona name cache |

### Building

Requirements: CMake 3.14 or newer, a C++17 compiler (MSVC), Git, and network access during configuration, because dependencies are fetched with `FetchContent`.

```
git clone https://github.com/OutTuna/Dota2CFGChanger.git
cd Dota2CFGChanger
cmake -S . -B build
cmake --build build --config Release
```

With a Visual Studio generator the executable is written to `build/Release/DotaManager.exe`. The first configuration also builds cURL and its dependencies through cpr, so it takes noticeably longer than later ones.

### Dependencies

All dependencies are fetched and built by CMake as static libraries.

| Library | Version | Purpose | License |
| --- | --- | --- | --- |
| [Dear ImGui](https://github.com/ocornut/imgui) | 1.91.6 | User interface | MIT |
| [GLFW](https://github.com/glfw/glfw) | 3.3.8 | Window, input, OpenGL context | zlib/libpng |
| [cpr](https://github.com/libcpr/cpr) | 1.11.1 | HTTP client (C++ wrapper over libcurl) | MIT |
| [nlohmann/json](https://github.com/nlohmann/json) | 3.11.3 | Settings and cache files | MIT |
| [stb](https://github.com/nothings/stb) | `master` | Image decoding (`stb_image`) | Public domain / MIT |

### Source layout

| Path | Role |
| --- | --- |
| `main.cpp` | Entry point and main loop |
| `ui.cpp`, `ui.h` | Rendering, themes, dialogs |
| `app_logic.cpp`, `app_logic.h` | Folder scan, config copy, settings, native folder dialog |
| `app_state.h` | Shared application state and file names |
| `avatar.cpp`, `avatar.h` | Asynchronous avatar download and OpenGL texture upload |
| `bg_crimson.h`, `app_icon.h`, `app.rc`, `Icon.ico` | Embedded resources |
| `CMakeLists.txt` | Build definition and dependency pinning |

### Limitations

- Windows only. The folder dialog and the default destination path are Windows-specific, and the CMake configuration links Windows libraries.
- The Steam installation is not detected. The default destination is the standard install path.
- Names and avatars require network access and a public profile.
- Interface strings are currently a mix of English and Russian.

### Issues

Bug reports and feature requests go to the [issue tracker](https://github.com/OutTuna/Dota2CFGChanger/issues). Please include the Windows version and the release you are running.

---

## Русский

### Описание

Dota 2 хранит раскладку клавиш, опции и другие клиентские настройки отдельно для каждого аккаунта, внутри каталога Steam `userdata`. Утилита копирует один этот каталог (app ID `570`) из папки одного аккаунта в папку другого. Настройки, собранные один раз, можно применить ко второму аккаунту, перенести на другой компьютер или восстановить из сохранённой копии.

Программа поставляется одним исполняемым файлом без установщика. Сторонние библиотеки собраны статически.

### Возможности

- Копируется только `<account_id>/570`. Остальное содержимое папки аккаунта не затрагивается.
- Сканирует исходный и целевой каталоги и показывает найденные в каждом папки аккаунтов.
- Показывает имена и аватары, полученные из публичных профилей Steam Community. Имена кэшируются локально.
- Перед заменой настроек запрашивает подтверждение.
- Пять тем оформления, выбор сохраняется между запусками.

### Требования

- Windows и видеодрайвер с поддержкой OpenGL.
- Каталог Steam `userdata` для назначения и каталог с такой же структурой для источника (это может быть один и тот же каталог).

### Использование

1. Скачайте `DotaManager.exe` в разделе [Releases](https://github.com/OutTuna/Dota2CFGChanger/releases).
2. Закройте Steam, чтобы клиент не писал в каталог в момент его замены.
3. Запустите программу. Нажмите на поле **Откуда конфиг** и выберите исходный каталог. Целевой каталог по умолчанию: `C:\Program Files (x86)\Steam\userdata`. Чтобы изменить его, нажмите на соответствующее поле.
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

**Изменяемые файлы.** Каталог `<account_id>/570` в назначении заменяется каталогом из источника. Операция деструктивная. Диалог подтверждения сообщает, что прежнее состояние можно вернуть только вручную из каталога `.bak`, создаваемого на время копирования. Если текущие настройки важны, сделайте собственную резервную копию.

**Сетевые запросы.** Для каждой найденной папки аккаунта программа запрашивает публичный XML профиля по адресу `steamcommunity.com/profiles/<SteamID64>?xml=1`, берёт из него имя и ссылку на аватар, затем загружает аватар. Передаётся только SteamID64, вычисленный из имени папки как `account_id + 76561197960265728`. Логин и API-ключ не используются. Для закрытых профилей имя и аватар недоступны, и в списке показывается числовой ID. Одновременно загружается не более четырёх аватаров.

**Локальные файлы.** В рабочий каталог записываются два JSON-файла:

| Файл | Содержимое |
| --- | --- |
| `settings.json` | путь к источнику, путь к назначению, выбранная тема |
| `nicknames.json` | кэш соответствия ID аккаунта и имени |

### Сборка

Требования: CMake 3.14 или новее, компилятор C++17 (MSVC), Git и доступ к сети на этапе конфигурации, поскольку зависимости загружаются через `FetchContent`.

```
git clone https://github.com/OutTuna/Dota2CFGChanger.git
cd Dota2CFGChanger
cmake -S . -B build
cmake --build build --config Release
```

При использовании генератора Visual Studio исполняемый файл создаётся по пути `build/Release/DotaManager.exe`. Первая конфигурация дополнительно собирает cURL и его зависимости через cpr, поэтому занимает заметно больше времени, чем последующие.

### Зависимости

Все зависимости загружаются и собираются CMake как статические библиотеки.

| Библиотека | Версия | Назначение | Лицензия |
| --- | --- | --- | --- |
| [Dear ImGui](https://github.com/ocornut/imgui) | 1.91.6 | Пользовательский интерфейс | MIT |
| [GLFW](https://github.com/glfw/glfw) | 3.3.8 | Окно, ввод, контекст OpenGL | zlib/libpng |
| [cpr](https://github.com/libcpr/cpr) | 1.11.1 | HTTP-клиент (обёртка над libcurl) | MIT |
| [nlohmann/json](https://github.com/nlohmann/json) | 3.11.3 | Файлы настроек и кэша | MIT |
| [stb](https://github.com/nothings/stb) | `master` | Декодирование изображений (`stb_image`) | Public domain / MIT |

### Структура исходников

| Путь | Назначение |
| --- | --- |
| `main.cpp` | Точка входа и главный цикл |
| `ui.cpp`, `ui.h` | Отрисовка, темы, диалоги |
| `app_logic.cpp`, `app_logic.h` | Сканирование папок, копирование, настройки, системный диалог выбора папки |
| `app_state.h` | Общее состояние приложения и имена файлов |
| `avatar.cpp`, `avatar.h` | Асинхронная загрузка аватаров и создание текстур OpenGL |
| `bg_crimson.h`, `app_icon.h`, `app.rc`, `Icon.ico` | Встроенные ресурсы |
| `CMakeLists.txt` | Описание сборки и версии зависимостей |

### Ограничения

- Только Windows. Диалог выбора папки и путь по умолчанию привязаны к Windows, а CMake-конфигурация подключает библиотеки Windows.
- Каталог установки Steam не определяется автоматически. В качестве целевого используется стандартный путь.
- Для имён и аватаров нужны сеть и публичный профиль.
- Строки интерфейса сейчас частично на английском, частично на русском.

### Обратная связь

Сообщения об ошибках и предложения принимаются в [трекере задач](https://github.com/OutTuna/Dota2CFGChanger/issues). Укажите версию Windows и используемый релиз.
