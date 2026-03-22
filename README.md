# Pulse Deck TS3

> This project is built with **OpenClaw** and **ChatGPT**.
>
> **Important attribution:** Pulse Deck TS3 is based on the original **RP Soundboard** TeamSpeak plugin codebase and continues from that foundation with rebrand and incremental refactor work.

**Pulse Deck TS3** is a standalone TeamSpeak 3 soundboard project focused on:

- playlist-first music workflow,
- detached **ButtonBox** control window,
- practical hotkey-driven live usage,
- ongoing UI/UX modernization.

> Current stage: **v0.0.1-alpha**

---

## Project status

Pulse Deck TS3 is an active alpha rewrite/rebrand.
The project started from the **RP Soundboard** codebase, and is being moved to its own identity, naming, packaging, and roadmap.

## Origin and attribution

- Original project baseline: **RP Soundboard** (TeamSpeak plugin)
- This repository keeps that functional lineage and evolves it under the **Pulse Deck TS3** identity.
- Existing licenses/attributions from the original project and third-party dependencies (including FFmpeg) must remain intact.

---

## Core goals

1. **Independent product identity**
   - New name, new repository, new package naming.

2. **Better live workflow**
   - Persistent playlist for music sessions.
   - Fast controls for start/stop/next/random.

3. **Detached ButtonBox window**
   - Button grid opened from a dedicated `ButtonBox` button.
   - Rows/Cols with `[-] [value] [+]` controls.
   - Auto-resize behavior as button count increases.

4. **Feature parity with legacy behavior**
   - Per-button hotkeys
   - Context menu actions (choose file, advanced, color, delete)
   - Drag & drop workflows
   - Per-sound settings (trim/crop/gain/preview)

---

## Current implemented features (alpha)

- Persistent playlist storage (JSON)
- Playlist multi-file import (`+ Files`)
- Playlist drag & drop
- Playlist remove selected entries (`- Remove`)
- Playlist random mode with non-repeating cycle behavior
- Main UI integration with detached ButtonBox (in progress hardening)
- TeamSpeak plugin build + package generation (`.ts3_plugin`)

---

## Planned/ongoing work

- Complete parity validation for all context-menu + advanced sound actions in detached ButtonBox flow
- Full settings/configuration behavior parity in the new layout
- Visual redesign pass (dark UI, clearer section hierarchy)
- Release pipeline cleanup and semantic version progression

---

## Build requirements (Windows)

- Visual Studio Build Tools 2022 (MSVC)
- CMake
- Qt 5.15.x (MSVC 64-bit)
- FFmpeg static libs expected under:
  - `ffmpeg/build/x86_64/include`
  - `ffmpeg/build/x86_64/lib`

---

## Build (example)

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH="C:/Qt/5.15.2/msvc2019_64"
cmake --build build --config Release --target INSTALL
```

If `RPSB_MAKE_PLUGIN_FILE=ON` is enabled, a `.ts3_plugin` package is generated in the configured output folder.

---

## Plugin identity (current)

- Plugin visible name: `Pulse Deck TS3`
- Command keyword: `pdeck`
- 64-bit DLL name: `pulse_deck_ts3_win64.dll`
- Package name pattern: `pulse_deck_ts3_<version>.ts3_plugin`
- Current documented stage: `v0.0.2-alpha`

## Output naming (Pulse Deck TS3)

- DLL: `pulse_deck_ts3_win64.dll`
- Package: `pulse_deck_ts3_<version>.ts3_plugin`
- Plugin visible name: `Pulse Deck TS3`

---

## Install on TeamSpeak 3 (Windows)

### Method A (recommended): install from `.ts3_plugin`

1. Download the latest package from Releases:
   - `pulse_deck_ts3_<version>.ts3_plugin`
2. Double-click the `.ts3_plugin` file.
3. Confirm the plugin installation dialog in TeamSpeak installer.
4. Start (or restart) TeamSpeak 3.
5. Open:
   - `Tools` -> `Options` -> `Addons`
6. Ensure **Pulse Deck TS3** is enabled.
7. Open plugin UI from the TeamSpeak Plugins/Addons menu.

### Method B: manual install (if double-click does not work)

1. Rename/copy the `.ts3_plugin` as `.zip` (or open it directly with an archive tool).
2. Extract package contents.
3. Copy files into your TeamSpeak user folder (typically `%AppData%\TS3Client\`), preserving structure:
   - `plugins\pulse_deck_ts3_win64.dll`
   - `plugins\pulse_deck_ts3\...` (assets)
   - `package.ini`
4. Restart TeamSpeak 3.
5. Check in `Tools` -> `Options` -> `Addons` that plugin is enabled.

### Troubleshooting

- Plugin not visible:
  - verify files are in the correct `%AppData%\TS3Client` path
  - restart TeamSpeak fully
- Plugin disabled at startup:
  - re-enable in Addons and restart
- Update prompt installs wrong version:
  - confirm release asset version name and internal plugin version match

---

## License / attribution

This project includes and builds upon prior open-source work and uses FFmpeg components under their respective licenses.
Please keep attribution and license texts intact where required.
