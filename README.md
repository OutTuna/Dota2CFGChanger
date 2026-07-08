# ⚡ Dota 2 Config Manager (C++ / ImGui)

![Windows](https://img.shields.io/badge/Windows-0078D6?style=flat&logo=windows&logoColor=white)
![C++](https://img.shields.io/badge/C%2B%2B-00599C?style=flat&logo=c%2B%2B&logoColor=white)
![Build](https://img.shields.io/badge/build-passing-brightgreen?style=flat)

**Dota 2 Config Manager** — это сверхбыстрая утилита для переноса настроек Dota 2 между разными Steam-аккаунтами.

Написана на **C++** с использованием графического движка **ImGui**. В отличие от скриптов на Python, эта программа весит всего пару мегабайт, запускается мгновенно и не требует установки дополнительных библиотек.

---

## 🖼 Скриншот

* **NEW**

![Interface Preview](https://github.com/user-attachments/assets/f5481271-bd9d-4093-a1b4-3382a1945173)

* **OLD (LINUX)**

![Interface Preview](https://github.com/user-attachments/assets/13438d84-9a6b-4355-8e7c-6a0a694b05a4)

---

## 🚀 Возможности

* **⚡ Мгновенная работа:** Запуск за 0.01 сек благодаря нативному C++.
* **🔍 Автопоиск аккаунтов:** Сканирует папки Steam `userdata` и находит все аккаунты.
* **🧠 Умное определение ников:** Автоматически загружает никнеймы через Steam API.
* **🎯 Хирургическая точность:** Переносит **только** настройки Dota 2 (папка `570`). Не трогает скриншоты, настройки других игр или данные аккаунта.
* **💾 Кэширование:** Запоминает ники после первого сканирования и работает офлайн.
* **📦 Полностью portable:** Один `.exe` файл. Никаких установок. Никаких лишних файлов рядом.

---

## ✨ Что нового

### v2.0 — Single-binary

Программа больше не создаёт **никаких файлов** рядом с собой:

| Раньше | Сейчас |
|---|---|
| `imgui.ini` — позиции окон | Хранится в памяти, не сохраняется |
| `settings.json` — пути и тема | Реестр Windows: `HKCU\Software\Dota2CFGChanger` |
| `nicknames.json` — кэш Steam API | Реестр Windows: `HKCU\Software\Dota2CFGChanger` |
| Сторонние DLL рядом с exe | Всё вшито в бинарник (Static Build + WinSSL) |

Настройки и кэш никнеймов теперь живут в реестре Windows — переместить exe в любую папку или на рабочий стол можно без потери данных. Удалить всё — одной командой:

```
reg delete HKCU\Software\Dota2CFGChanger /f
```

---

## 🛠 Технический стек

Проект собирается в статический бинарный файл (Static Build), все зависимости вшиты внутрь:

* **[ImGui](https://github.com/ocornut/imgui)** — Графический интерфейс (Immediate Mode GUI).
* **[GLFW](https://github.com/glfw/glfw)** — Работа с окнами и OpenGL.
* **[cpr](https://github.com/libcpr/cpr)** — C++ Wrapper для cURL (сетевые запросы).
* **[nlohmann/json](https://github.com/nlohmann/json)** — Парсинг JSON для Steam API.
* **Windows SChannel** — Встроенный TLS Windows вместо OpenSSL.
* **Windows Registry API** — Хранение настроек и кэша без файлов на диске.

---

## 📥 Как пользоваться

1. Скачайте последнюю версию `DotaManager.exe` из раздела **[Releases](../../releases)**.
2. Запустите программу — никаких установок, никаких DLL рядом.
3. Укажите пути (обычно они определяются автоматически):
    * **Source Path:** Папка, где лежат ваши заготовленные конфиги.
    * **Dest Path:** Папка `userdata` в Steam (`C:\Program Files (x86)\Steam\userdata`).
4. Нажмите **SCAN FOLDERS**.
5. Выберите в левой колонке **ЧЕЙ** конфиг взять.
6. Выберите в правой колонке **В КАКОЙ** аккаунт загрузить.
7. Нажмите **COPY CONFIG NOW**.
8. Готово! Можно запускать Доту.

---

![Alt](https://repobeats.axiom.co/api/embed/a71213f8cd667b684ab859eea88dce13aea336bc.svg "Repobeats analytics image")

---

## 🏗 Сборка из исходников (Build from source)

Потребуется **CMake** и компилятор **MSVC** (MinGW тоже поддерживается).

```bash
# 1. Клонируйте репозиторий
git clone https://github.com/OutTuna/Dota2CFGChanger.git
cd Dota2CFGChanger

# 2. Создайте папку сборки
cmake -S . -B build

# 3. Скомпилируйте в режиме Release
cmake --build build --config Release
```

На выходе — один `DotaManager.exe` без каких-либо зависимостей рядом.