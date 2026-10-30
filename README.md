# AhoiPlank

A minimal Wayland application launcher (wofi/rofi - like) built with Qt 6 and QML. Bootstrapped with `AhoiCpp`.
It contains currently fuzzy search, `.desktop` file parsing, icon theme support, layer-shell overlay and single-instance toggling.

## How to build

I plan to have two options. Native and devcontainer.
The devcontainer is under construction right now, but it will work at some time.

### Native build dependencies

For a native build, you will need:
- Qt 6.5+ (`qt6-base`, `qt6-declarative`, `qt6-wayland`, `qt6-svg`)
- LayerShellQt (`layer-shell-qt` on Arch, `liblayer-shell-qt-dev` on Ubuntu)
- CMake 3.28+, Ninja, a C++23 compiler (GCC 16+ or Clang 18+)
- Wayland and wayland-protocols, libxkbcommon

Install on Arch:

```bash
sudo pacman -S qt6-base qt6-declarative qt6-wayland qt6-svg layer-shell-qt cmake ninja gcc clang wayland wayland-protocols libxkbcommon
```

Install on Ubuntu:

```bash
sudo apt install qt6-base-dev qt6-declarative-dev qt6-wayland qt6-svg-dev liblayer-shell-qt-dev cmake ninja-build g++-16 wayland-protocols libxkbcommon-dev
```

### Devcontainer dependencies

Docker with the BuildKit/builder that ships with modern Docker (or `docker buildx`). No other host dependencies — the container provides the toolchains.

### Build with `build.py`

The `build.py` script drives CMake. You will get prompted to a menu, so you can set your build to release, debug, add a version and so on.

```bash
python3 build.py
```

Binaries land in `build/App/AhoiPlank`.

### Build with the devcontainer

```bash
sh start_devcontainer.sh
```

This builds the `ahoiplank-dev` image and drops you into a shell inside it, with all toolchains and Qt installs available. From inside the container:

```bash
sh cross_compile_in_devcontainer.sh
```

This runs `build.py` for every `Arm64` and `x86` Linux and produces:

- `cross-build/linux-x86_64/App/AhoiPlank`
- `cross-build/linux-arm64/App/AhoiPlank`

Note: The image build takes 30–40 minutes on first run since Qt needs to be built from source for each target. It gets cached afterward.

## Running the launcher

Of course you need to be on `Wayland`.
Then you can easily go to the directory where `AhoiPlank` is compiled and run, for example:

```bash
cd build/App
./AhoiPlank
```

If it works correctly, you can later on install on your system:

```bash
sudo cp build/App/AhoiPlank /usr/local/bin/ahoiplank
```

Then you will be able to add it to your keybindings on your window manager.
