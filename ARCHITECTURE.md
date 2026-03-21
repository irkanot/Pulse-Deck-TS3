# Pulse Deck TS3 - Architecture Map

## 1) Runtime flow (high level)

1. TeamSpeak loads plugin DLL (`*_win32.dll` / `*_win64.dll`).
2. TS3 Plugin SDK callbacks enter through `src/plugin.cpp`.
3. Core orchestration in `src/main.cpp` routes actions:
   - open UI
   - play/stop/pause
   - hotkeys
   - command parsing
4. UI (`src/MainWindow.*`) updates config and triggers playback actions.
5. Sampler/audio engine (`src/samples.*`) manages playback state and sample production.
6. FFmpeg input layer (`src/inputfileffmpeg.cpp`) decodes media files.
7. Audio frames are injected through TS3 capture/playback hooks.

---

## 2) Main modules and dependencies

### Entry / TS3 integration
- `src/plugin.cpp`
  - Implements TS3-required plugin functions and event callbacks.
  - Depends on TeamSpeak SDK headers in `pluginsdk/include`.
  - Delegates behavior to `src/main.cpp`.

- `src/main.cpp`, `src/main.h`
  - Central coordinator for plugin actions.
  - Exposes functions used by UI/hotkeys:
    - `sb_playButton`, `sb_stopPlayback`, `sb_pauseButtonPressed`, etc.
  - Talks to MainWindow, ConfigModel, Sampler, UpdateChecker.

### UI / interaction
- `src/MainWindow.cpp`, `src/MainWindow.h`, `src/MainWindow.ui`
  - Main plugin window.
  - Playlist controls, settings, hotkey menus, button interactions.
  - Calls core `sb_*` functions.

- `src/SoundButton.*`
  - Per-button UI behavior (click, context menu, drag&drop).

- `src/SoundSettings.*`
  - Advanced sound dialog (trim/crop/gain/preview-related UI).

- `src/ExpandableSection.*`
  - Collapsible sections (Settings / Configurations).

### Configuration / model
- `src/ConfigModel.*`
  - Persistent configuration state.
  - Volumes, flags, active profile/config, button metadata.
  - Observer pattern used by UI.

- `src/SoundInfo.*`
  - Metadata per sound/button (filename, custom options, etc.).

### Audio pipeline
- `src/samples.*` (Sampler)
  - Playback state machine and stream handling.

- `src/SampleProducerThread.*`, `src/SampleBuffer.*`, `src/SampleVisualizerThread.*`
  - Producer/consumer pipeline and buffering.

- `src/TalkStateManager.*`
  - Handles talking/mute behavior during playback.

- `src/inputfile.h`, `src/inputfileffmpeg.cpp`
  - File input abstraction + FFmpeg decoder implementation.

### Update system
- `src/UpdateChecker.*`
  - Reads latest GitHub release info and proposes updates.

---

## 3) Build and packaging chain

### Source/build system
- `CMakeLists.txt`
- `files.cmake`
- `version.py` (generates version header/package data from tags)

### External deps
- `pluginsdk/` -> TeamSpeak SDK headers
- `ffmpeg/` -> scripts/submodule/build outputs for codec support

### Packaging
- `src/package.ini.in` -> package metadata template
- `create-pluginfile.cmake` -> creates `.ts3_plugin`
- `deploy/` -> deployment/package content

---

## 4) Hotkey/event flow example

1. TS3 emits hotkey event -> `plugin.cpp` callback.
2. Routed into `sb_onHotkeyPressed` (`main.cpp`).
3. `main.cpp` maps keyword:
   - button hotkey -> `sb_playButton(...)`
   - stop/pause -> corresponding sampler command
   - playlist hotkeys -> `MainWindow` playlist methods
4. Sampler starts/stops playback.
5. UI receives sampler signals (start/stop/pause) and refreshes state.

---

## 5) Planned new component (for rebrand)

### `ButtonBoxWindow` (separate window)
Recommended new module set:
- `src/ButtonBoxWindow.h`
- `src/ButtonBoxWindow.cpp`
- optional `src/ButtonBoxWindow.ui`

Responsibilities:
- Detached grid window for sound buttons.
- Rows/Cols controls with `[-] [value] [+]`.
- Auto-resize to keep minimum button size.
- Preserve existing button hotkey and drag/drop behavior.

Integration points:
- MainWindow keeps only an "Open ButtonBox" button.
- ConfigModel stores ButtonBox geometry + rows/cols.
- Sound button actions still call `sb_*` core playback functions.

---

## 6) Immediate next implementation steps

1. Add `ButtonBoxWindow` class and wire open/close from MainWindow.
2. Move sound-grid creation logic from MainWindow to ButtonBoxWindow.
3. Add Rows/Cols minus/plus controls and persist state in ConfigModel.
4. Implement auto-resize rule + optional scroll fallback.
5. Keep feature parity for context menu, advanced settings, hotkeys, drag/drop.
