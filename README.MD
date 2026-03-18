# Dota 2 CFG Changer 🐧 Linux

Утилита для копирования конфигов Dota 2 между Steam-аккаунтами.

> **Ты в ветке `linux`.**  
> Для Windows-версии переключись на ветку **`main`**.

---

## Ветки

| Ветка    | Платформа  | Описание                        |
|----------|------------|---------------------------------|
| `main`   | 🪟 Windows | `.exe`-сборка, пути Windows     |
| `linux`  | 🐧 Linux   | бинарник Linux, пути `~/.steam` |

---

## Скачать готовый бинарник

1. Перейди во вкладку **Actions** → выбери последний успешный билд
2. Внизу страницы найди раздел **Artifacts**
3. Скачай `Dota2CFGChanger-linux`

---

## Запуск

```bash
# Даём права на выполнение
chmod +x Dota2CFGChanger

# Запускаем
./Dota2CFGChanger
```

---

## Зависимости (runtime)

```bash
# Ubuntu / Debian
sudo apt install libglfw3 libgl1-mesa-glx libcurl4

# Arch
sudo pacman -S glfw-x11 libgl curl

# Fedora
sudo dnf install glfw mesa-libGL libcurl
```

---

## Пути Steam по умолчанию

Программа автоматически определяет путь при первом запуске:

| Установка Steam | Путь к userdata |
|-----------------|-----------------|
| Обычная         | `~/.local/share/Steam/userdata` |
| Flatpak         | `~/.var/app/com.valvesoftware.Steam/data/Steam/userdata` |
| Snap            | `~/snap/steam/common/.local/share/Steam/userdata` |

Если автоопределение не сработало — вставь путь вручную в поле **Dest Path**.

---

## Сборка из исходников

```bash
# Установка зависимостей
sudo apt install cmake build-essential libglfw3-dev libgl1-mesa-dev \
                 nlohmann-json3-dev libcurl4-openssl-dev

# Клонирование с submodules (imgui)
git clone --recurse-submodules https://github.com/OutTuna/Dota2CFGChanger.git
cd Dota2CFGChanger
git checkout linux

# Сборка cpr
git clone https://github.com/libcpr/cpr.git && cd cpr
mkdir build && cd build && cmake .. -DCPR_USE_SYSTEM_CURL=ON && make -j$(nproc)
sudo make install && cd ../..

# Сборка проекта
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```