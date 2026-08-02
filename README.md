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
