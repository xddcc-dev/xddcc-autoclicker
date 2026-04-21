# how to build xddcc autoclicker on linux??

## requirements

make sure to install these:

```bash
# Ubuntu / Debian
sudo apt install \
    cmake ninja-build \
    qt6-base-dev qt6-multimedia-dev \
    libx11-dev libxtst-dev \
    pkg-config

# Arch / Manjaro
sudo pacman -S cmake ninja qt6-base qt6-multimedia libx11 libxtst pkgconf

# Fedora
sudo dnf install cmake ninja-build qt6-qtbase-devel qt6-qtmultimedia-devel \
    libX11-devel libXtst-devel pkgconf
```

## Build

```bash
cd XDDCCClicker

cmake -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH=/usr/lib/x86_64-linux-gnu/qt6   # adjust if Qt6 is elsewhere

cmake --build build
```

the binary will be at `build/XDDCCAutoClicker`.

## Run

```bash
./build/XDDCCAutoClicker
```

if you get a platform plugin error:
```bash
export QT_QPA_PLATFORM=xcb
./build/XDDCCAutoClicker
```

## just so u know

- click simulation uses **XTest** (part of libXtst). This works on X11.
- On **Wayland** youll need to run under XWayland: `QT_QPA_PLATFORM=xcb ./build/XDDCCAutoClicker`
- The **window picker** only works on X11 (the Win32 EnumWindows equivalent is not implemented for Linux yet, it will show an empty list).
- **PostMessage** click method is only for windows on Linux it doesnt work (use SendInput which maps to XTest)
- global hotkey registration (`RegisterHotKey`) is only for windows, on Linux the hotkey field in settings has no effect currently. u can use your desktop environments shortcut manager to bind a key to send a signal to the process if needed.
