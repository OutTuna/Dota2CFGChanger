# Dota 2 CFG Changer 🐧 Linux

Утилита для копирования конфигов Dota 2 между Steam-аккаунтами.

> **Ты в ветке `linux`.**  
> Для Windows-версии переключись на ветку **`main`**.

---

## Ветки

| Ветка    | Платформа  | Описание                        |
|----------|------------|---------------------------------|
| `main`   | 🪟 Windows | `.exe`-сборка, пути Windows     |
| `linux`  | 🐧 Linux   | AppImage, пути `~/.steam`       |

---

## Скачать

1. Перейди во вкладку **Actions** → выбери последний успешный билд
2. Внизу страницы найди раздел **Artifacts**
3. Скачай `Dota2CFGChanger-linux`
4. Распакуй `.zip` — внутри будет `Dota2CFGChanger-linux.AppImage`

---

## Запуск

AppImage не требует установки и работает на любом Linux без зависимостей.

```bash
# Даём права на выполнение (один раз)
chmod +x Dota2CFGChanger-linux.AppImage

# Запускаем
./Dota2CFGChanger-linux.AppImage
```

Или просто дважды кликни по файлу в файловом менеджере.

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
# Зависимости (только компилятор и OpenGL — остальное скачается само)

# Ubuntu / Debian
sudo apt install cmake build-essential libgl1-mesa-dev libx11-dev \
                 libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev

# Fedora
sudo dnf install cmake gcc-c++ mesa-libGL-devel libX11-devel \
                 libXrandr-devel libXinerama-devel libXcursor-devel libXi-devel

# Arch
sudo pacman -S cmake base-devel mesa libx11 libxrandr libxinerama libxcursor libxi

# Сборка
git clone https://github.com/OutTuna/Dota2CFGChanger.git
cd Dota2CFGChanger
git checkout linux
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release   # ImGui, cpr, json, GLFW скачаются сами
make -j$(nproc)
```
