#include <amxmodx>
#include <amxmisc>
#include <memstore>

#define PLUGIN_NAME    "MemStore Example: Dynamic Loadout & Copied Size"
#define PLUGIN_VERSION "1.0.0"
#define PLUGIN_AUTHOR  "iceeedR"

#define MAX_LOADOUT_ITEMS 10

// Namespace for player loadout sessions
new const LOADOUT_NS[] = "player_loadouts";

// Friendly weapon names for logging
new const g_szWeaponNames[][] = {
	"None", "P228", "Shield", "Scout", "HE Grenade", "XM1014", "C4",
	"MAC10", "AUG", "Smoke Grenade", "Elite", "Fiveseven", "UMP45",
	"SG550", "Galil", "Famas", "USP", "Glock18", "AWP", "MP5",
	"M249", "M3", "M4A1", "TMP", "G3SG1", "Flashbang", "Deagle", "SG552", "AK47", "Knife", "P90"
};

public plugin_init()
{
	register_plugin(PLUGIN_NAME, PLUGIN_VERSION, PLUGIN_AUTHOR);

	// Client chat commands to save and inspect loadouts
	register_clcmd("say /saveloadout", "CmdSaveLoadout");
	register_clcmd("say /loadout", "CmdInspectLoadout");
	register_clcmd("say_team /saveloadout", "CmdSaveLoadout");
	register_clcmd("say_team /loadout", "CmdInspectLoadout");

	// Console command
	register_concmd("amx_loadout", "CmdInspectLoadout", ADMIN_ALL, "Inspect saved player loadout in RAM");

	server_print("==================================================");
	server_print("[MemStore-Loadout] Example Dynamic Loadout plugin loaded.");
	server_print("  Commands:");
	server_print("    say /saveloadout  - Saves mock loadout of variable length");
	server_print("    say /loadout      - Reads loadout using copied_size");
	server_print("==================================================");
}

// --------------------------------------------------
// Auto-Restore on Connection
// --------------------------------------------------
public client_putinserver(id)
{
	if (is_user_hltv(id))
	{
		return;
	}

	RestorePlayerLoadout(id);
}

// --------------------------------------------------
// Core Logic: Dynamic-Length Array with copied_size
// --------------------------------------------------

/**
 * Saves a variable-length item list into MemStore RAM.
 * Only the active item count is saved (not the empty slots).
 */
SaveCustomLoadout(id, const items[], count)
{
	new authid[MAX_AUTHID_LENGTH];
	get_user_authid(id, authid, charsmax(authid));

	// Retain loadout in RAM for 1 map rotation with zero disk I/O
	// Only 'count' cells are written into MemStore
	mem_set_array(LOADOUT_NS, authid, items, count, EXP_MAP_COUNT, 1);

	client_print(id, print_chat, "[MemStore] Saved %d equipment item(s) to RAM for next map.", count);
}

/**
 * Restores a variable-length item list from MemStore RAM.
 * Demonstrates the '&copied_size' parameter:
 * - Buffer has MAX_LOADOUT_ITEMS (10) cells.
 * - 'actualCount' receives the exact number of valid cells stored.
 * - Prevents iterating over uninitialized/garbage slots.
 */
RestorePlayerLoadout(id)
{
	new authid[MAX_AUTHID_LENGTH];
	get_user_authid(id, authid, charsmax(authid));

	new loadoutBuffer[MAX_LOADOUT_ITEMS];
	new actualCount = 0;

	// 'actualCount' is populated with the true number of elements copied
	if (!mem_get_array(LOADOUT_NS, authid, loadoutBuffer, sizeof(loadoutBuffer), actualCount))
	{
		return;
	}

	if (actualCount == 0)
	{
		return;
	}

	client_print(id, print_chat, "[MemStore] Restored %d equipment item(s) from previous map:", actualCount);

	// Iterate strictly up to 'actualCount', never touching unused buffer capacity
	for (new i = 0; i < actualCount; i++)
	{
		new weaponId = loadoutBuffer[i];
		if (0 <= weaponId < sizeof(g_szWeaponNames))
		{
			client_print(id, print_chat, "  - Slot #%d: %s (ID: %d)", i + 1, g_szWeaponNames[weaponId], weaponId);
		}
		else
		{
			client_print(id, print_chat, "  - Slot #%d: Unknown (ID: %d)", i + 1, weaponId);
		}
	}
}

// --------------------------------------------------
// Commands
// --------------------------------------------------
public CmdSaveLoadout(id)
{
	if (!is_user_connected(id))
	{
		return PLUGIN_HANDLED;
	}

	// Mock dynamic loadout: 3 items (AK-47, Deagle, Flashbang)
	new sampleItems[3];
	sampleItems[0] = 28; // AK47
	sampleItems[1] = 26; // Deagle
	sampleItems[2] = 25; // Flashbang

	SaveCustomLoadout(id, sampleItems, sizeof(sampleItems));
	return PLUGIN_HANDLED;
}

public CmdInspectLoadout(id)
{
	if (!is_user_connected(id))
	{
		return PLUGIN_HANDLED;
	}

	new authid[MAX_AUTHID_LENGTH];
	get_user_authid(id, authid, charsmax(authid));

	new loadoutBuffer[MAX_LOADOUT_ITEMS];
	new itemsCopied = 0;

	if (mem_get_array(LOADOUT_NS, authid, loadoutBuffer, sizeof(loadoutBuffer), itemsCopied))
	{
		client_print(id, print_chat, "[MemStore] Active Loadout: %d / %d slots utilized in RAM.",
			itemsCopied, MAX_LOADOUT_ITEMS);

		for (new i = 0; i < itemsCopied; i++)
		{
			new weaponId = loadoutBuffer[i];
			if (0 <= weaponId < sizeof(g_szWeaponNames))
			{
				client_print(id, print_chat, "  #%d: %s", i + 1, g_szWeaponNames[weaponId]);
			}
			else
			{
				client_print(id, print_chat, "  #%d: Unknown", i + 1);
			}
		}
	}
	else
	{
		client_print(id, print_chat, "[MemStore] No loadout saved in RAM. Type /saveloadout first.");
	}

	return PLUGIN_HANDLED;
}
