# 🚀 YimMenuV2

**Experimental mod menu for GTA 5: Enhanced**

![Status](https://img.shields.io/badge/status-experimental-orange)
![Platform](https://img.shields.io/badge/platform-GTA%20V%20Enhanced-blue)
![Language](https://img.shields.io/badge/language-C%2B%2B-red)
![License](https://img.shields.io/badge/license-MIT-green)


## 📌 Overview

**YimMenuV2** is an experimental mod menu for **Grand Theft Auto V: Enhanced Edition**, built as a modern successor to the original YimMenu.  
It provides powerful features, stability improvements, and an extensible architecture for future development.

> ⚠️ **Disclaimer:** This project is for educational and research purposes only.  
> Using mod menus in online sessions may violate Rockstar's Terms of Service.


## 📥 Installation Guide

### 🔧 Requirements

- GTA V: Enhanced Edition
- DLL Injector (e.g., Xenos)
- Optional but recommended: **FSL**


### 📝 Step-by-Step Setup

1. **Download FSL (Recommended)**  
   👉 [Download Here](https://www.unknowncheats.me/forum/grand-theft-auto-v/616977-fsl-local-gtao-saves.html)  
   - Place `WINMM.dll` inside your **GTA V root directory**  
   - Using FSL improves **account safety**

2. **Download YimMenuV2**  
   👉 [GitHub Releases](https://github.com/CSY0N/YimMenuVersion2/releases)

3. **Download a DLL Injector**  
   Recommended: [Xenos Injector](https://www.unknowncheats.me/forum/general-programming-and-reversing/124013-xenos-injector-v2-3-2-a.html)

4. **Disable BattlEye**
   - Open **Rockstar Launcher**
   - Select **GTA V Enhanced**
   - Go to **Settings → Disable BattlEye**
   - Steam / Epic users may need:
     ```
     -nobattleye
     ```

5. **Launch GTA V**
   - Stay at the **main menu**
   - Inject **Yimura.Menu.Enhanced.dll**


## 🎮 Controls

| Action | Key |
|--------|------|
| Open Menu | `INSERT` or `CTRL + \` |


## ❗ Common Issues & Fixes


### 🔄 Desync Every 5 Minutes

> **Problem:** Getting kicked or desynced from public sessions.

**Cause:** No current BattlEye bypass — legitimate hosts detect heartbeat failure.

**Solution:**  
- Use **invite-only / private sessions**
- Use a **proper BattlEye bypass** (not provided)


### 💾 Progress Disappeared After Removing FSL

> **Problem:** Progress is missing after removing FSL.

**Explanation:**  
FSL stores account data locally. Removing it hides those saves.

**Fix Options:**
- Keep **FSL enabled**
- Or avoid using FSL entirely *(not recommended)*


### ❌ Game Won't Start After Removing FSL

> **Problem:** GTA crashes or won't launch.

**Fix:**  
Delete this folder in Appdata: FSL

### I removed FSL and the game doesn't start up anymore

This is a known issue; delete "Documents/GTAV Enhanced/Profiles" to fix

## TinkerScripts

**TinkerScripts v1.0.1 starts automatically with the menu.** Its feature callbacks, widgets, data and cloud-save helper are compiled into C++; there are no TinkerScripts Lua files to install or select.

### Menu categories

The native integration registers **96 commands across seven categories**:

- Main
- Biz-Teroids
- Phone Master
- Gun Van Halen
- Rainbow Vehicles
- Recovery +
- Settings +

The port retains the original **GTA V Enhanced 1158.16 / 1.73** build check and game offsets. Other Lua scripts continue to use the menu's existing Lua system.

### Settings and exports

Runtime data is stored in `%AppData%/YimMenuV2/TinkerScripts/`:

| File | Purpose |
| :--- | :--- |
| `Settings.json` | Saved feature preferences. |
| `Exported_TattooCombo.txt` | Tattoo import/export data. |
| `NoSave.txt` | Cloud-save helper status, created when needed. |

If a current saved copy does not exist, the port reads earlier TinkerScripts settings and tattoo exports as data. Existing files in the old scripts folder are left in place.

### Cloud-save tool

1. Open **Settings + $\rightarrow$ Start Cloud Save Tool**.
2. Approve the Windows elevation prompt.
3. Choose **Disable Cloud Saves** or **Enable Cloud Saves** in the helper window.

The helper runs the same DLL through Windows' `rundll32` and uses Windows Firewall COM APIs. It retains the original rule name and outbound IP list, and removes its rule when closed or when the game exits. Close an older batch tool before starting this helper so both tools do not control the same rule.

### Source layout

| File | Responsibility |
| :--- | :--- |
| `src/core/scripting/TinkerScript.cpp` | Feature callbacks, UI, game bindings, settings and the firewall helper. |
| `src/core/scripting/TinkerScript.hpp` | Startup and shutdown entry points. |
| `src/core/scripting/TinkerScriptData.cpp` | Blip, tunable, unlock, tattoo and default-settings tables. |
| `src/core/scripting/TinkerScriptData.hpp` | Shared values, table keys and callback state. |
| `src/core/scripting/TinkerScriptssTests.cpp` | Optional game-free regression checks. |
| `cmake/tinker-scripts.cmake` | Compiler options and the optional test target. |

The shared value containers preserve the original table and callback behavior. They do not interpret or execute Lua source. ImGui enums retain their numeric flag and index values.

### Verification and credits

To run the optional checks after configuring either compiler, enable the test target in that build directory:

```powershell
cmake -S . -B build/clang -DBUILD_SCRIPT_TESTS=ON
cmake --build build/clang --target TinkerScriptssTests --parallel 2
.\build\clang\TinkerScriptssTests.exe .\test-output-clang-new
```

For MSVC, substitute `build/msvc` and a different new output directory. Each test command requires a directory that does **not** already exist. Set `BUILD_SCRIPT_TESTS=OFF` again for routine menu builds.

The optional checks cover command registration, table and numeric behavior, ImGui enum bindings, representative reversible callbacks, settings migration, JSON persistence and default category rendering using headless ImGui with mocked game APIs. These checks do not verify every gameplay action. Live GTA behavior and elevated firewall changes remain untested.

Original script: [ImagineNothing/TinkerScripts](https://github.com/lmagineNothing/TinkerScripts). The native conversion is AI-assisted. The original GPLv3-or-later text and dependency notices are retained in the existing [LICENSE](LICENSE), alongside the menu's original GPLv2 text. Retaining these notices does not resolve compatibility between the grants; combined licensing requires maintainer review before merging or releasing the port.
