# AMX Mod X MemStore

[![Build Status](https://github.com/NiceFeatures/amxx-memstore/actions/workflows/build.yml/badge.svg)](https://github.com/NiceFeatures/amxx-memstore/actions/workflows/build.yml)
[![Latest Release](https://img.shields.io/github/v/release/NiceFeatures/amxx-memstore?color=blue&label=release)](https://github.com/NiceFeatures/amxx-memstore/releases/latest)
[![Standard](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.wikipedia.org/wiki/C%2B%2B17)
[![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20Linux-brightgreen.svg)]()
[![Compatibility](https://img.shields.io/badge/AMXX-1.9%20%7C%201.10%20%7C%20ReAMXX-orange.svg)]()

**MemStore** is a high-performance in-memory key-value data storage module for **AMX Mod X (GoldSrc / HLDS)** designed for ultra-fast transient cross-plugin and cross-map data retention.

Unlike `nVault` (which incurs disk I/O latency) or `localinfo` (which is severely constrained by engine string size limits in GoldSrc), **MemStore** operates directly in RAM with zero disk I/O, namespace isolation, strict type inspection, deterministic sorted rankings (ZSETs), and flexible expiration lifecycles.

---

## ⚡ Features

- **RAM-Only Persistence**: Survives map changes (`changelevel`) with zero disk I/O.
- **Multiple Data Types**: Native support for Integers (`cell`), Floats (`Float:`), Strings (`string`), and Arrays (`array[]`).
- **Native Leaderboard & Ranking Engine (ZSET)**:
  - In-RAM sorted sets with instant score lookup (`mem_rank_set`, `mem_rank_get_top`, `mem_rank_get_pos`).
  - Supports descending order (`RANK_DESC`) for kills/points and ascending order (`RANK_ASC`) for speedruns/race timers.
  - Deterministic tie-breaking for equal scores.
- **Zero-Handle Key Enumeration**:
  - `mem_get_key_at(namespace, index, dest, maxlen)` and `mem_get_namespace_count(namespace)`.
  - Iterate through keys in a namespace without allocating or tracking Pawn handles.
- **Namespace Sandboxing**: Multiple plugins can safely coexist without key name collisions.
- **Flexible Expiration Policies**:
  - `EXP_PERSISTENT`: Retained in RAM across map changes until server reboot.
  - `EXP_MAP_END`: Automatically wiped on the next map change with zero manual cleanup.
  - `EXP_MAP_COUNT`: Retained for $N$ consecutive map rotations.
  - `EXP_TTL`: Wall-clock Time-To-Live expiration (in seconds).
- **Type Safety**: Runtime type detection and inspection via `mem_get_key_type()`.
- **Anti-DoS Protection**: Built-in limits on key counts per namespace and max array sizes.
- **Self-Contained SDK**: Includes required AMX Mod X SDK headers for immediate, standalone compilation.
- **Universal Linux Compatibility**: Compiled in an Ubuntu 18.04 container with `-D_GLIBCXX_USE_CXX11_ABI=0` for 100% compatibility across both legacy and modern Linux servers.

---

## 📦 Downloads & Installation

Pre-compiled binaries and complete package archives are generated automatically for every release:

👉 **[Download Latest Release Packages](https://github.com/NiceFeatures/amxx-memstore/releases/latest)**

* **`memstore-universal.zip`** *(Recommended)*: Contains both Windows (`.dll`) and Linux (`.so`) binaries, Pawn headers (`memstore.inc`), and example scripts.
* **`memstore-win32.zip`**: Windows 32-bit module package.
* **`memstore-linux32.zip`**: Linux 32-bit module package.

### Directory Structure

Extract the archive into your server's `cstrike/` folder:

```plaintext
addons/
└── amxmodx/
    ├── modules/
    │   ├── memstore_amxx.dll        (Windows)
    │   └── memstore_amxx_i386.so   (Linux)
    └── scripting/
        ├── include/
        │   └── memstore.inc
        ├── test_memstore.sma        (Unit test suite & benchmark runner)
        └── example_top15.sma        (Example Top 15 stats plugin with MOTD)
```

Enable the module in `addons/amxmodx/configs/modules.ini`:
```ini
memstore
```

---

## 💡 Pawn Usage Examples

### Example 1: Basic Cross-Map RAM Persistence
```pawn
#include <amxmodx>
#include <memstore>

public client_putinserver(id)
{
    new authid[35];
    get_user_authid(id, authid, charsmax(authid));

    // Store integer to survive across 2 map changes
    mem_set_int("session_scores", authid, 100, EXP_MAP_COUNT, 2);

    // Store array in RAM across map changes
    new gear[3] = { CSW_AK47, CSW_DEAGLE, CSW_FLASHBANG };
    mem_set_array("player_gear", authid, gear, sizeof(gear), EXP_PERSISTENT);
}

public OnNewMap()
{
    new authid[35];
    get_user_authid(1, authid, charsmax(authid));

    new score = 0;
    if (mem_get_int("session_scores", authid, score))
    {
        server_print("[MemStore] Restored previous map score from RAM: %d", score);
    }
}
```

### Example 2: In-RAM Map Top 15 Leaderboard
```pawn
#include <amxmodx>
#include <memstore>

public OnPlayerKilled(victim, killer)
{
    new killerName[32];
    get_user_name(killer, killerName, charsmax(killerName));

    new currentKills = 0;
    mem_rank_get_score("map_kills", killerName, currentKills);

    // Automatically ranked in RAM; EXP_MAP_END auto-clears on map change
    mem_rank_set("map_kills", killerName, currentKills + 1, EXP_MAP_END);
}

public ShowTopLeader(id)
{
    new leaderName[32], topKills = 0;
    if (mem_rank_get_top("map_kills", 1, leaderName, charsmax(leaderName), topKills, RANK_DESC))
    {
        client_print(id, print_chat, "[MemStore] Current 1st Place: %s with %d kills!", leaderName, topKills);
    }
}
```

---

## 🛠️ Building from Source

### Windows (Visual Studio / CMake)

Run:
```cmd
build.bat
```
Or manually:
```cmd
cmake -B build -A Win32 -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```
Output: `build/Release/memstore_amxx.dll`

---

### Linux (Makefile / GCC)

Install 32-bit compilation dependencies:
```bash
sudo dpkg --add-architecture i386
sudo apt-get update
sudo apt-get install -y gcc-multilib g++-multilib make
```
Build:
```bash
make
```
Output: `build_linux/memstore_amxx_i386.so`

---

## 🚀 CI / CD Automation

The repository includes a GitHub Actions workflow (`.github/workflows/build.yml`) that automatically:
1. Compiles the Linux 32-bit module in an isolated `ubuntu:18.04` container with `-D_GLIBCXX_USE_CXX11_ABI=0`.
2. Compiles the Windows 32-bit module using MSVC.
3. Automatically generates `.zip` deployment archives and publishes a GitHub Release for every tag or commit on `main`.

---

## 📄 License

Author: **iceeedR**  
Licensed under the [GNU General Public License v3.0](https://www.gnu.org/licenses/gpl-3.0.html).
