# AhoiPlank

A minimal Wayland application launcher (wofi/rofi - like) built with Qt 6 and QML. Bootstrapped with `AhoiCpp`.
It contains currently fuzzy search, `.desktop` file parsing, icon theme support, layer-shell overlay and single-instance toggling.

## How to build

I plan to have two options. Native and devcontainer.
The devcontainer is under construction right now, but it will work at some time.

### Native build dependencies

For a native build, you will need:
- Qt 6.5+ (`qt6-base`, `qt6-declarative`, `qt6-wayland`, `qt6-svg`)
- LayerShellQt (`layer-shell-qt` on Arch, `liblayershellqtinterface-dev` on Ubuntu)
- CMake 3.28+, Ninja, a C++23 compiler (GCC 16+ or Clang 18+)
- Wayland and wayland-protocols, libxkbcommon

Install on Arch:

```bash
sudo pacman -S qt6-base qt6-declarative qt6-wayland qt6-svg layer-shell-qt cmake ninja gcc clang wayland wayland-protocols libxkbcommon
```

Install on Ubuntu:

```bash
sudo apt install qt6-base-dev qt6-declarative-dev qt6-wayland libqt6svg6-dev liblayershellqtinterface-dev cmake ninja-build g++-16 wayland-protocols libxkbcommon-dev
```

### Build with `build.py`

The `build.py` script drives CMake. You will get prompted to a menu, so you can set your build to release, debug, add a version and so on.

```bash
python3 build.py
```

Binaries land in `build/App/AhoiPlank`.

## Devcontainer

The devcontainer provides a ready-to-use environment with Qt 6, LayerShellQt, and a C++23 compiler. No host dependencies beyond Docker.

### Requirements

Docker with BuildKit (or `docker buildx`). No other host dependencies — the container provides the toolchains.

### Build the devcontainer and enter it

```bash
sh start_devcontainer.sh
```

This builds the `ahoiplank-dev` image for your host architecture and drops you into a shell inside it. The image build takes about 2 minutes on first run and is cached afterward.

### Build AhoiPlank inside the devcontainer

From inside the container:

```bash
cd /workspace
python3 build.py
```

The binary lands at `/workspace/build/App/AhoiPlank`, which is your project directory mounted from the host.

## Running the launcher

You need to be on `Wayland`.
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
