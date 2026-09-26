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
- **Dynamic Arrays & Schema Validation**: `mem_get_array()` provides an optional `&copied_size` output parameter returning the exact count of cells copied. Ideal for variable-sized lists (inventories, weapon loadouts) and struct-size schema validation.
- **Unified Single-Namespace Architecture**: Each namespace independently maintains both a Key-Value table and a Sorted Rank Leaderboard. Composite arrays and leaderboard ranks can share the exact same namespace and entity key (`authid`) without collision.
- **Native Leaderboard & Ranking Engine (ZSET)**:
  - In-RAM sorted sets with instant score lookup (`mem_rank_set`, `mem_rank_get_top`, `mem_rank_get_pos`).
  - Supports descending order (`RANK_DESC`) for kills/points and ascending order (`RANK_ASC`) for speedruns/race timers.
  - Deterministic tie-breaking for equal scores.
- **Zero-Handle Key Enumeration**:
  - Index-based key lookup: `mem_get_key_at(namespace, index, dest, maxlen)` and `mem_get_namespace_count(namespace)`.
  - Public callback iteration: `mem_iterate_keys(namespace, "MyCallback")`.
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
        ├── example_top15.sma        (Top 15 deaths ranking with MOTD)
        ├── example_mapstats.sma     (Full map stats & intermission MOTD via ReAPI)
        └── example_loadout.sma      (Dynamic loadout & copied_size showcase)
```

Enable the module in `addons/amxmodx/configs/modules.ini` (optional, as `#pragma loadlib` autoloads automatically):
```ini
memstore
```

---

## 💡 Pawn Usage Examples

### Example 1: Dynamic Arrays & `copied_size` Validation
```pawn
#include <amxmodx>
#include <memstore>

#define MAX_LOADOUT_SLOTS 10

public SavePlayerLoadout(id)
{
    new authid[MAX_AUTHID_LENGTH];
    get_user_authid(id, authid, charsmax(authid));

    // Player currently has 3 active items
    new activeGear[3] = { CSW_AK47, CSW_DEAGLE, CSW_FLASHBANG };

    // Save only the 3 active items (variable-length array in RAM)
    mem_set_array("player_gear", authid, activeGear, sizeof(activeGear), EXP_MAP_COUNT, 2);
}

public RestorePlayerLoadout(id)
{
    new authid[MAX_AUTHID_LENGTH];
    get_user_authid(id, authid, charsmax(authid));

    new gearBuffer[MAX_LOADOUT_SLOTS];
    new itemsRestored = 0;

    // 'itemsRestored' receives the exact number of cells copied (here: 3)
    if (mem_get_array("player_gear", authid, gearBuffer, sizeof(gearBuffer), itemsRestored))
    {
        server_print("[MemStore] Restored %d active items for %s:", itemsRestored, authid);

        // Iterate safely only over the restored elements without reading garbage
        for (new i = 0; i < itemsRestored; i++)
        {
            server_print("  - Item #%d: CSW ID %d", i + 1, gearBuffer[i]);
        }
    }
}
```

### Example 2: Unified Namespace Leaderboard + Composite Record
```pawn
#include <amxmodx>
#include <memstore>

new const MAP_STATS_NS[] = "mapstats";

enum _:PlayerStats
{
    STAT_KILLS = 0,
    STAT_DEATHS,
    STAT_NAME[MAX_NAME_LENGTH] // Embedded string in the cell array
};

public SavePlayerSession(id)
{
    new authid[MAX_AUTHID_LENGTH];
    get_user_authid(id, authid, charsmax(authid));

    new pData[PlayerStats];
    pData[STAT_KILLS]  = get_user_frags(id);
    pData[STAT_DEATHS] = get_user_deaths(id);
    get_user_name(id, pData[STAT_NAME], charsmax(pData[][STAT_NAME]));

    // 1. Store full composite record (stats + name) in Key-Value store
    mem_set_array(MAP_STATS_NS, authid, pData, sizeof(pData), EXP_MAP_END);

    // 2. Rank player by kills under the EXACT SAME namespace
    mem_rank_set(MAP_STATS_NS, authid, pData[STAT_KILLS], EXP_MAP_END);
}

public ShowTopLeader(id)
{
    new topAuth[MAX_AUTHID_LENGTH], topKills = 0;

    // Fetch #1 from sorted leaderboard
    if (mem_rank_get_top(MAP_STATS_NS, 1, topAuth, charsmax(topAuth), topKills, RANK_DESC))
    {
        // Retrieve full record and nickname in a single call without secondary lookups
        new pData[PlayerStats], copied = 0;
        if (mem_get_array(MAP_STATS_NS, topAuth, pData, sizeof(pData), copied) && copied == PlayerStats)
        {
            client_print(id, print_chat, "[MemStore] #1: %s (%d Kills, %d Deaths)",
                pData[STAT_NAME], pData[STAT_KILLS], pData[STAT_DEATHS]);
        }
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
