# Carafe

A KDE Plasma-native game launcher for Windows games via Proton/UMU. Built with Qt 6.6+ and Kirigami.

<table>
  <tr>
    <td><img src="res/screenshots/grid.png" height="500"></td>
    <td><img src="res/screenshots/addgame.png" height="500"></td>
  </tr>
</table>

## Build

```bash
cmake -B build
cmake --build build
./build/bin/carafe
```

Dependencies: CMake 4.3+, C++17 compiler, Qt 6.6+, KF6 (Kirigami, CoreAddons), KF6 Wallet (optional), `icoutils` (optional), `umu-launcher` (optional).

Install system-wide with `sudo cmake --install build`.

## Data

- Library: `~/.local/share/io.marlonn.carafe/library.json`
- Prefixes: `~/carafe/prefixes/<game-slug>/`

## Packaging

- Arch: `cd packaging/arch && makepkg -si`
