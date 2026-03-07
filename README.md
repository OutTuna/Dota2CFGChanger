# ⚡ Dota 2 Config Manager (C++ / ImGui)

![Platform](https://img.shields.io/badge/platform-Windows-blue)
![Language](https://img.shields.io/badge/language-C%2B%2B17-00599C)
![License](https://img.shields.io/badge/license-MIT-green)
![Build](https://img.shields.io/badge/build-passing-brightgreen)

**Dota 2 Config Manager** — это сверхбыстрая утилита для переноса настроек Dota 2 между разными Steam-аккаунтами. 

Написана на **C++** с использованием графического движка **ImGui**. В отличие от скриптов на Python, эта программа весит всего пару мегабайт, запускается мгновенно и не требует установки дополнительных библиотек.

---

## 🖼 Скриншот

![Interface Preview](https://github.com/user-attachments/assets/13438d84-9a6b-4355-8e7c-6a0a694b05a4)

---

## 🚀 Возможности

* **⚡ Мгновенная работа:** Запуск за 0.01 сек благодаря нативному C++.
* **🔍 Автопоиск аккаунтов:** Сканирует папки Steam `userdata` и находит все аккаунты.
* **🧠 Умное определение ников:** Автоматически загружает никнеймы через Steam API.
* **🎯 Хирургическая точность:** Переносит **только** настройки Dota 2 (папка `570`). Не трогает скриншоты, настройки других игр или данные аккаунта.
* **💾 Кэширование:** Запоминает ники после первого сканирования и работает офлайн.
* **📦 Portable:** Один `.exe` файл. Никаких установок.

---

## 🛠 Технический стек

Проект собран в статический бинарный файл (Static Build), все зависимости вшиты внутрь:

* **[ImGui](https://github.com/ocornut/imgui)** — Графический интерфейс (Immediate Mode GUI).
* **[GLFW](https://github.com/glfw/glfw)** — Работа с окнами и OpenGL.
* **[cpr](https://github.com/libcpr/cpr)** — C++ Wrapper для cURL (сетевые запросы).
* **[nlohmann/json](https://github.com/nlohmann/json)** — Парсинг JSON для настроек.

---

## 📥 Как пользоваться

1.  Скачайте последнюю версию `DotaManager.exe` из раздела **[Releases](../../releases)**.
2.  Запустите программу.
3.  Укажите пути (обычно они определяются автоматически):
    * **Source Path:** Папка, где лежат ваши заготовленные конфиги.
    * **Dest Path:** Папка `userdata` в Steam (`C:\Program Files (x86)\Steam\userdata`).
4.  Нажмите **SCAN FOLDERS**.
5.  Выберите в левой колонке **ЧЕЙ** конфиг взять.
6.  Выберите в правой колонке **В КАКОЙ** аккаунт загрузить.
7.  Нажмите **COPY CONFIG NOW**.
8.  Готово! Можно запускать Доту.

---

![Alt](https://repobeats.axiom.co/api/embed/a71213f8cd667b684ab859eea88dce13aea336bc.svg "Repobeats analytics image")


---

## 🏗 Сборка из исходников (Build from source)

Если вы хотите собрать проект самостоятельно, вам понадобится **CMake** и компилятор C++ (MSVC).

```bash
# 1. Клонируйте репозиторий
git clone [https://github.com/your-username/dota-config-manager.git](https://github.com/your-username/dota-config-manager.git)
cd dota-config-manager

# 2. Создайте папку сборки
cmake -S . -B build

# 3. Скомпилируйте (в режиме Release для макс. скорости)
cmake --build build --config Release





