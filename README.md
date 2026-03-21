# Pulse Deck TS3

**Pulse Deck TS3** is a standalone TeamSpeak 3 soundboard project focused on:

- playlist-first music workflow,
- detached **ButtonBox** control window,
- practical hotkey-driven live usage,
- ongoing UI/UX modernization.

> Current stage: **v0.0.1-alpha**

---

## Project status

Pulse Deck TS3 is an active alpha rewrite/rebrand.
The project started from an existing codebase, but it is now being moved to its own identity, naming, packaging, and roadmap.

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

## Output naming (Pulse Deck TS3)

- DLL: `pulse_deck_ts3_win64.dll`
- Package: `pulse_deck_ts3_<version>.ts3_plugin`
- Plugin visible name: `Pulse Deck TS3`

---

## License / attribution

This project includes and builds upon prior open-source work and uses FFmpeg components under their respective licenses.
Please keep attribution and license texts intact where required.
