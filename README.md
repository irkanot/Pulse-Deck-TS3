# RP Soundboard Playlist (Irk Fork)

An enhanced fork of **RP Soundboard** for TeamSpeak 3 with a modern playlist workflow, random playback modes, and GitHub-based update delivery.

This fork keeps the classic soundboard button system, but adds a **separate persistent playlist** designed for music sessions.

---

## What this fork adds

### 1) Independent persistent playlist (new)
The playlist is now independent from the button grid.

- You can maintain a music queue without assigning tracks to soundboard buttons.
- Playlist content is stored persistently in a local JSON file.
- The playlist is reloaded automatically when the plugin starts.

### 2) Add tracks with multi-select (new)
A dedicated **`+ Files`** button lets you import one or many tracks at once.

- Opens a multi-file picker.
- Adds selected files to the playlist.
- Saves playlist immediately.

### 3) Drag & drop into playlist (new)
You can drag audio files directly into the playlist UI.

- Dropped files are added to the playlist.
- Playlist file is updated immediately.

### 4) Playlist controls (new)
Dedicated controls are available in the playlist section:

- **Play Playlist** (start/stop)
- **Prev**
- **Next**
- **Random ON/OFF** (toggle mode)

### 5) True random mode with no repeats (new)
Random is now a **mode flag**, not a one-shot action.

When **Random ON**:

- First track (from Play Playlist) is random.
- Next track is random.
- Auto-advance after track end is random.
- Tracks are not repeated until all tracks have been played once.

### 6) Button playback no longer hijacks playlist flow (fix)
Pressing a manual soundboard button now correctly:

- interrupts current playback,
- plays the selected button track,
- and **does not** accidentally trigger playlist-next behavior.

### 7) GitHub Release updater integration (new)
Update checks were adapted for this fork.

- The plugin checks this repository’s latest release.
- Update prompt points to this fork’s release package (`.ts3_plugin`).

Repository: `https://github.com/irkanot/RP-Soundboard-Irk-Fork`

---

## Original RP Soundboard features (still included)

- Broad media format support (mp3, wav, flac, ogg, mp4, etc.)
- Per-sound trimming/cropping
- Per-sound volume controls
- Hotkeys for buttons and controls

---

## Installation

### Windows
1. Download the latest `.ts3_plugin` from this fork’s Releases page.
2. Double-click the file and follow TeamSpeak installer prompts.

If double-click does not work:
- drag & drop the `.ts3_plugin` onto `package_inst.exe` in your TeamSpeak installation folder.

### Linux
Use `package_inst` from your TeamSpeak installation directory:

```bash
LD_LIBRARY_PATH=.:$LD_LIBRARY_PATH ./package_inst <path-to-plugin.ts3_plugin>
```

Alternatively, open the plugin file as an archive and extract `plugins/*` to:

`~/.ts3client/plugins/`

---

## Notes

- This project is based on the original work by Marius Gräfe.
- This fork focuses on playlist-first music management while preserving the classic soundboard workflow.
