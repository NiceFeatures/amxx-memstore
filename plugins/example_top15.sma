#include <amxmodx>
#include <amxmisc>
#include <memstore>

#define PLUGIN_NAME    "MemStore Example: Top 15 Map Stats"
#define PLUGIN_VERSION "1.0.0"
#define PLUGIN_AUTHOR  "iceeedR"

// Single namespace for both native leaderboard and auxiliary player stats in RAM
// EXP_MAP_END automatically clears entries on map change with zero manual cleanup
new const TOP15_NS[] = "map_deaths_top15";

enum _:PlayerStats
{
	STAT_KILLS = 0,
	STAT_DEATHS,
	STAT_KNIFE_DEATHS
};

public plugin_init()
{
	register_plugin(PLUGIN_NAME, PLUGIN_VERSION, PLUGIN_AUTHOR);

	register_clcmd("say /top15", "CmdShowTop15");
	register_clcmd("say /deaths", "CmdShowTop15");
	register_clcmd("say_team /top15", "CmdShowTop15");

	register_concmd("amx_top15", "CmdShowTop15Server", ADMIN_ALL, "Display Top 15 Map Deaths Ranking");
	register_srvcmd("amx_top15_mock", "CmdPopulateMock", -1, "Regenerate mock ranking data in RAM");

	PopulateMockData();

	server_print("==================================================");
	server_print("[MemStore-Top15] Example Top 15 Stats plugin loaded.");
	server_print("  Commands:");
	server_print("    amx_top15           - View MOTD table in console");
	server_print("    amx_top15_mock      - Repopulate mock data");
	server_print("    say /top15          - Client MOTD window");
	server_print("==================================================");
}

public CmdPopulateMock()
{
	PopulateMockData();
	server_print("[MemStore-Top15] Mock data re-populated successfully in RAM.");
	return PLUGIN_HANDLED;
}

PopulateMockData()
{
	mem_rank_clear(TOP15_NS);
	mem_clear_namespace(TOP15_NS);

	new const mockNames[][] = {
		"coldzera", "FalleN", "fer", "TACO", "fnx",
		"s1mple", "ZywOo", "NiKo", "device", "ropz",
		"m0NESY", "b1t", "karrigan", "rain", "broky",
		"shox", "kennyS", "f0rest", "GeT_RiGhT", "Olofmeister"
	};

	new const mockStats[][] = {
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

		// 1. Store in native sorted leaderboard under the single namespace
		mem_rank_set(TOP15_NS, mockNames[i], deaths, EXP_MAP_END);

		// 2. Store detailed player stats array under the same namespace and key
		new pData[PlayerStats];
		pData[STAT_KILLS]        = mockStats[i][STAT_KILLS];
		pData[STAT_DEATHS]       = mockStats[i][STAT_DEATHS];
		pData[STAT_KNIFE_DEATHS] = mockStats[i][STAT_KNIFE_DEATHS];

		mem_set_array(TOP15_NS, mockNames[i], pData, sizeof(pData), EXP_MAP_END);
	}
}

BuildTop15Motd(motdBuffer[], maxlen)
{
	new totalCount = mem_rank_get_count(TOP15_NS);
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
	new pData[PlayerStats];
	new copied = 0;

	for (new rankPos = 1; rankPos <= limit; rankPos++)
	{
		if (!mem_rank_get_top(TOP15_NS, rankPos, playerName, charsmax(playerName), deaths, RANK_DESC))
		{
			break;
		}

		new kills = 0, knifeDeaths = 0;
		if (mem_get_array(TOP15_NS, playerName, pData, sizeof(pData), copied) && copied == PlayerStats)
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

public CmdShowTop15(id)
{
	new motd[2048];
	BuildTop15Motd(motd, charsmax(motd));
	show_motd(id, motd, "Top 15 Map Deaths Ranking");
	return PLUGIN_HANDLED;
}

public CmdShowTop15Server(id, level, cid)
{
	new motd[2048];
	BuildTop15Motd(motd, charsmax(motd));

	if (id == 0)
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
