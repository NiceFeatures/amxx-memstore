#include <amxmodx>
#include <amxmisc>
#include <memstore>

#define PLUGIN_NAME    "MemStore Example: Top 15 Map Stats"
#define PLUGIN_VERSION "1.0.0"
#define PLUGIN_AUTHOR  "iceeedR"

// Namespace for the native leaderboard and player stats in RAM
// Notice EXP_MAP_END: both automatically reset on map change with zero manual cleanup!
new const RANK_NS[]  = "map_deaths_top15";
new const STATS_NS[] = "map_player_stats";

// Array indices for stats_summary
enum
{
	STAT_KILLS = 0,
	STAT_DEATHS,
	STAT_KNIFE_DEATHS
};

public plugin_init()
{
	register_plugin(PLUGIN_NAME, PLUGIN_VERSION, PLUGIN_AUTHOR);

	// Client chat commands to open the MOTD
	register_clcmd("say /top15", "CmdShowTop15");
	register_clcmd("say /deaths", "CmdShowTop15");
	register_clcmd("say_team /top15", "CmdShowTop15");

	// Server console and admin commands
	register_concmd("amx_top15", "CmdShowTop15Server", ADMIN_ALL, "Display Top 15 Map Deaths Ranking");
	register_srvcmd("amx_top15_mock", "CmdPopulateMock", -1, "Regenerate mock ranking data in RAM");

	// Populate mock player stats into MemStore RAM
	PopulateMockData();

	server_print("==================================================");
	server_print("[MemStore-Top15] Example Top 15 Stats plugin loaded.");
	server_print("  Commands:");
	server_print("    amx_top15           - View MOTD table in console");
	server_print("    amx_top15_mock      - Repopulate mock data");
	server_print("    say /top15          - Client MOTD window");
	server_print("==================================================");
}

// --------------------------------------------------
// MOCK DATA GENERATOR
// Demonstrates storing native sorted leaderboards + extra arrays in RAM
// --------------------------------------------------
public CmdPopulateMock()
{
	PopulateMockData();
	server_print("[MemStore-Top15] Mock data re-populated successfully in RAM.");
	return PLUGIN_HANDLED;
}

PopulateMockData()
{
	// Clear previous entries in RAM
	mem_rank_clear(RANK_NS);
	mem_clear_namespace(STATS_NS);

	// 20 Mock players (top 15 will be displayed, remaining 5 illustrate truncation)
	new const mockNames[][] = {
		"coldzera", "FalleN", "fer", "TACO", "fnx",
		"s1mple", "ZywOo", "NiKo", "device", "ropz",
		"m0NESY", "b1t", "karrigan", "rain", "broky",
		"shox", "kennyS", "f0rest", "GeT_RiGhT", "Olofmeister"
	};

	new const mockStats[][] = {
		// Kills, Deaths, KnifeDeaths
		{ 35, 45, 4 }, // coldzera
		{ 28, 42, 6 }, // FalleN
		{ 30, 39, 2 }, // fer
		{ 22, 38, 5 }, // TACO
		{ 25, 36, 3 }, // fnx
		{ 45, 34, 1 }, // s1mple
		{ 40, 33, 2 }, // ZywOo
		{ 38, 30, 0 }, // NiKo
		{ 29, 28, 1 }, // device
		{ 31, 27, 2 }, // ropz
		{ 33, 25, 3 }, // m0NESY
		{ 26, 24, 1 }, // b1t
		{ 18, 23, 4 }, // karrigan
		{ 24, 22, 2 }, // rain
		{ 27, 21, 1 }, // broky
		{ 20, 19, 0 }, // shox
		{ 23, 18, 1 }, // kennyS
		{ 19, 16, 2 }, // f0rest
		{ 17, 15, 0 }, // GeT_RiGhT
		{ 15, 12, 1 }  // Olofmeister
	};

	for (new i = 0; i < sizeof(mockNames); i++)
	{
		new deaths = mockStats[i][STAT_DEATHS];

		// 1. Store in the native Sorted Rank Leaderboard (sorted by deaths DESC)
		// EXP_MAP_END ensures zero manual cleanup needed when map changes
		mem_rank_set(RANK_NS, mockNames[i], deaths, EXP_MAP_END);

		// 2. Store detailed player stats array in RAM under the player name
		new pData[3];
		pData[STAT_KILLS]        = mockStats[i][STAT_KILLS];
		pData[STAT_DEATHS]       = mockStats[i][STAT_DEATHS];
		pData[STAT_KNIFE_DEATHS] = mockStats[i][STAT_KNIFE_DEATHS];

		mem_set_array(STATS_NS, mockNames[i], pData, sizeof(pData), EXP_MAP_END);
	}
}

// --------------------------------------------------
// MOTD BUILDER
// Reads top 15 from native ZSET and combines with RAM arrays
// --------------------------------------------------
BuildTop15Motd(motdBuffer[], maxlen)
{
	new totalCount = mem_rank_get_count(RANK_NS);
	new limit = (totalCount > 15) ? 15 : totalCount;

	new currentMap[64];
	get_mapname(currentMap, charsmax(currentMap));

	new len = 0;
	len += formatex(motdBuffer[len], maxlen - len, "==========================================================^n");
	len += formatex(motdBuffer[len], maxlen - len, "   TOP 15 MAP DEATHS & STATS - %s^n", currentMap);
	len += formatex(motdBuffer[len], maxlen - len, "   (Powered by MemStore In-RAM Storage)^n");
	len += formatex(motdBuffer[len], maxlen - len, "==========================================================^n");
	len += formatex(motdBuffer[len], maxlen - len, "%-4s %-20s %-8s %-8s %-12s^n", "#", "Player Name", "Deaths", "Kills", "Knife Deaths");
	len += formatex(motdBuffer[len], maxlen - len, "----------------------------------------------------------^n");

	new playerName[32];
	new deaths = 0;
	new pData[3];
	new copied = 0;

	for (new rankPos = 1; rankPos <= limit; rankPos++)
	{
		// Query top ranks natively in descending order (highest deaths first)
		if (!mem_rank_get_top(RANK_NS, rankPos, playerName, charsmax(playerName), deaths, RANK_DESC))
		{
			break;
		}

		// Retrieve associated stats array from RAM
		new kills = 0, knifeDeaths = 0;
		if (mem_get_array(STATS_NS, playerName, pData, sizeof(pData), copied) && copied == 3)
		{
			kills = pData[STAT_KILLS];
			knifeDeaths = pData[STAT_KNIFE_DEATHS];
		}

		len += formatex(motdBuffer[len], maxlen - len,
			"%-4d %-20s %-8d %-8d %-12d^n",
			rankPos, playerName, deaths, kills, knifeDeaths);
	}

	len += formatex(motdBuffer[len], maxlen - len, "==========================================================^n");
	len += formatex(motdBuffer[len], maxlen - len, " Total Players Tracked in RAM: %d^n", totalCount);
	len += formatex(motdBuffer[len], maxlen - len, " Memory Policy: EXP_MAP_END (Auto-cleared on map change)^n");
	len += formatex(motdBuffer[len], maxlen - len, "==========================================================^n");
}

// --------------------------------------------------
// COMMAND HANDLERS
// --------------------------------------------------
public CmdShowTop15(id)
{
	new motd[2048];
	BuildTop15Motd(motd, charsmax(motd));

	// Display plain-text MOTD popup to player
	show_motd(id, motd, "Top 15 Map Deaths Ranking");
	return PLUGIN_HANDLED;
}

public CmdShowTop15Server(id, level, cid)
{
	new motd[2048];
	BuildTop15Motd(motd, charsmax(motd));

	if (id == 0) // Server console
	{
		server_print("^n%s", motd);
	}
	else
	{
		console_print(id, "%s", motd);
		show_motd(id, motd, "Top 15 Map Deaths Ranking");
	}
	return PLUGIN_HANDLED;
}
