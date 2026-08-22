# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]

### Added

- Grid cards: hover highlight, single-click select, double-click launch.
- Keyboard navigation (arrow keys + Enter), scrollbars, and an empty-library message in both views.
- Right-click menu for list rows
- API key show/hide toggle
- Minimum window size.

### Changed

- The grid/list view choice is remembered across restarts.
- Error notifications stay visible longer (8s vs 4s)
- Consistent menu labels and icons.
- Artwork keeps its real file extension.
- GAMEID is only sent to umu-run when a UMU ID is configured.

### Fixed

- "Fetch Artwork" no longer gets stuck on failure and shows the error in the dialog.
- Games without artwork show the placeholder in list view instead of a blank icon.
- Removing a running game no longer freezes the window.
- SteamGridDB failures report the actual cause (e.g. invalid API key) instead of "No games found".
- A corrupt library.json is quarantined as library.json.corrupt-<timestamp> instead of being overwritten.
- Launching an already-running game gives feedback

## [0.1.3] - 2026-08-17

### Added

- Launch wrapper support (e.g. `game-performance`, `gamemoderun`) with a global default in Settings and a per-game override in Add/Edit Game. Wrappers apply to game launches, installers, and "Run exe in prefix".

### Changed

- AddGameDialog does not close when clicking the empty space around it anymore.

### Removed

- The "Extra Proton search paths" setting. Proton builds are now only discovered in the standard locations.

## [0.1.2] - 2026-07-10

### Changed

- File picker now uses xdg-file-picker, should look nicer

### Fixed

- Installer text should now reset color even if install fails

## [0.1.1] - 2026-07-05


## [0.1.0] - 2026-07-05


### Added

- None

### Changed

- None.

### Fixed

- None.

### Removed

- None.
