#include <amxmodx>
#include <amxmisc>
#include <reapi>
#include <memstore>

#define PLUGIN_NAME    "MemStore Example: Map Stats & Intermission"
#define PLUGIN_VERSION "1.0.0"
#define PLUGIN_AUTHOR  "iceeedR"

#define TASK_INTERMISSION_MOTD 4821

// Single namespace: MemStore holds independent Key-Value and Rank tables per namespace
new const MAP_STATS_NS[] = "mapstats";

// Unified player record: numeric stats + embedded nickname (37 cells total)
enum _:PlayerStats
{
	STAT_KILLS = 0,
	STAT_DEATHS,
	STAT_KNIFE_KILLS,
	STAT_C4_PLANTS,
	STAT_C4_DEFUSES,
	STAT_NAME[MAX_NAME_LENGTH]
};

// Runtime per-client cache
new g_ePlayerStats[MAX_PLAYERS + 1][PlayerStats];
new g_szAuth[MAX_PLAYERS + 1][MAX_AUTHID_LENGTH];
new bool:g_bStatsLoaded[MAX_PLAYERS + 1];

// Map intermission state flag
new bool:g_bIntermissionStarted = false;

public plugin_init()
{
	register_plugin(PLUGIN_NAME, PLUGIN_VERSION, PLUGIN_AUTHOR);

	// Track kills, deaths, and knife kills
	register_event_ex("DeathMsg", "Event_DeathMsg", RegisterEvent_Global);

	// ReAPI hooks for successful C4 objectives
	RegisterHookChain(RG_PlantBomb, "RG_PlantBomb_Post", .post = true);
	RegisterHookChain(RG_CGrenade_DefuseBombEnd, "RG_BombDefused_Post", .post = true);

	// ReAPI hook for map end / intermission
	RegisterHookChain(RG_CSGameRules_GoToIntermission, "CSGameRules_GoToIntermission_Post", .post = true);

	// Player commands
	register_clcmd("say /stats", "Cmd_ShowStats");
	register_clcmd("say /mapstats", "Cmd_ShowStats");
	register_clcmd("say /top", "Cmd_ShowStats");
	register_clcmd("say_team /stats", "Cmd_ShowStats");
	register_clcmd("say_team /mapstats", "Cmd_ShowStats");
	register_clcmd("say_team /top", "Cmd_ShowStats");

	// Console command
	register_clcmd("amx_mapstats", "Cmd_ShowStats");

	server_print("[MemStore-Stats] Loaded successfully (Namespace: '%s' | EXP_MAP_END).", MAP_STATS_NS);
}

public plugin_end()
{
	remove_task(TASK_INTERMISSION_MOTD);
}

// --------------------------------------------------
// Client Lifecycle
// --------------------------------------------------
public client_putinserver(id)
{
	ResetPlayerLocalData(id);

	if (is_user_hltv(id))
	{
		return;
	}

	get_user_authid(id, g_szAuth[id], charsmax(g_szAuth[]));

	// Restore session data from RAM if the player reconnected during the same map
	new copied = 0;
	if (mem_get_array(MAP_STATS_NS, g_szAuth[id], g_ePlayerStats[id], sizeof(g_ePlayerStats[]), copied) && copied == PlayerStats)
	{
		get_user_name(id, g_ePlayerStats[id][STAT_NAME], charsmax(g_ePlayerStats[][STAT_NAME]));
	}
	else
	{
		arrayset(g_ePlayerStats[id], 0, sizeof(g_ePlayerStats[]));
		get_user_name(id, g_ePlayerStats[id][STAT_NAME], charsmax(g_ePlayerStats[][STAT_NAME]));
	}

	g_bStatsLoaded[id] = true;
	SavePlayerStats(id);

	if (g_bIntermissionStarted)
	{
		set_task(0.5, "Task_ShowSinglePlayerMotd", id);
	}
}

public client_disconnected(id)
{
	if (g_bStatsLoaded[id])
	{
		SavePlayerStats(id);
	}

	remove_task(id);
	ResetPlayerLocalData(id);
}

public client_infochanged(id)
{
	if (!is_user_connected(id) || !g_bStatsLoaded[id])
	{
		return;
	}

	new szNewName[MAX_NAME_LENGTH];
	get_user_info(id, "name", szNewName, charsmax(szNewName));

	if (!equal(g_ePlayerStats[id][STAT_NAME], szNewName))
	{
		copy(g_ePlayerStats[id][STAT_NAME], charsmax(g_ePlayerStats[][STAT_NAME]), szNewName);
		SavePlayerStats(id);
	}
}

ResetPlayerLocalData(id)
{
	arrayset(g_ePlayerStats[id], 0, sizeof(g_ePlayerStats[]));
	g_szAuth[id][0] = EOS;
	g_bStatsLoaded[id] = false;
}

// --------------------------------------------------
// MemStore Synchronization
// --------------------------------------------------
SavePlayerStats(id)
{
	if (!g_bStatsLoaded[id] || g_szAuth[id][0] == EOS)
	{
		return;
	}

	// Store full composite record (counters + nickname) and update kill ranking under 1 namespace
	mem_set_array(MAP_STATS_NS, g_szAuth[id], g_ePlayerStats[id], sizeof(g_ePlayerStats[]), EXP_MAP_END);
	mem_rank_set(MAP_STATS_NS, g_szAuth[id], g_ePlayerStats[id][STAT_KILLS], EXP_MAP_END);
}

// --------------------------------------------------
// Gameplay Events
// --------------------------------------------------
public Event_DeathMsg()
{
	new attacker = read_data(1);
	new victim   = read_data(2);

	if (1 <= victim <= MaxClients && is_user_connected(victim) && g_bStatsLoaded[victim])
	{
		g_ePlayerStats[victim][STAT_DEATHS]++;
		SavePlayerStats(victim);
	}

	if (1 <= attacker <= MaxClients && is_user_connected(attacker) && attacker != victim && g_bStatsLoaded[attacker])
	{
		if (1 <= victim <= MaxClients && is_user_connected(victim))
		{
			if (get_member(attacker, m_iTeam) == get_member(victim, m_iTeam))
			{
				return;
			}
		}

		g_ePlayerStats[attacker][STAT_KILLS]++;

		new szWeapon[24];
		read_data(4, szWeapon, charsmax(szWeapon));

		if (equal(szWeapon, "knife"))
		{
			g_ePlayerStats[attacker][STAT_KNIFE_KILLS]++;
		}

		SavePlayerStats(attacker);
	}
}

public RG_PlantBomb_Post(const id, Float:vecStart[3], Float:vecVelocity[3])
{
	if (1 <= id <= MaxClients && is_user_connected(id) && g_bStatsLoaded[id])
	{
		g_ePlayerStats[id][STAT_C4_PLANTS]++;
		SavePlayerStats(id);
	}

	return HC_CONTINUE;
}

public RG_BombDefused_Post(const thisEntity, const defuser, bool:bDefused)
{
	if (bDefused && 1 <= defuser <= MaxClients && is_user_connected(defuser) && g_bStatsLoaded[defuser])
	{
		g_ePlayerStats[defuser][STAT_C4_DEFUSES]++;
		SavePlayerStats(defuser);
	}

	return HC_CONTINUE;
}

// --------------------------------------------------
// Intermission & MOTD
// --------------------------------------------------
public CSGameRules_GoToIntermission_Post()
{
	g_bIntermissionStarted = true;

	// Broadcast SVC_FINALE to display intermission view
	message_begin(MSG_ALL, SVC_FINALE);
	write_string("");
	message_end();

	// Short delay allows intermission camera to settle before opening MOTD
	remove_task(TASK_INTERMISSION_MOTD);
	set_task(0.3, "Task_ShowIntermissionMotd", TASK_INTERMISSION_MOTD);

	return HC_CONTINUE;
}

public Task_ShowIntermissionMotd()
{
	new players[MAX_PLAYERS], num;
	get_players_ex(players, num, GetPlayers_ExcludeHLTV);

	for (new i = 0; i < num; i++)
	{
		ShowPlayerStatsMotd(players[i]);
	}
}

public Task_ShowSinglePlayerMotd(id)
{
	if (is_user_connected(id))
	{
		ShowPlayerStatsMotd(id);
	}
}

public Cmd_ShowStats(id)
{
	if (!is_user_connected(id))
	{
		return PLUGIN_HANDLED;
	}

	ShowPlayerStatsMotd(id);
	return PLUGIN_HANDLED;
}

// --------------------------------------------------
// MOTD Construction
// --------------------------------------------------
ShowPlayerStatsMotd(id)
{
	new szMotd[1536];
	BuildStatsHtml(id, szMotd, charsmax(szMotd));
	show_motd(id, szMotd, "Map Stats");
}

BuildStatsHtml(id, buffer[], maxlen)
{
	new szCurrentMap[32];
	get_mapname(szCurrentMap, charsmax(szCurrentMap));

	new myRank = 0;
	if (g_szAuth[id][0] != EOS)
	{
		myRank = mem_rank_get_pos(MAP_STATS_NS, g_szAuth[id], RANK_DESC);
	}

	new Float:flKD = (g_ePlayerStats[id][STAT_DEATHS] > 0)
		? (float(g_ePlayerStats[id][STAT_KILLS]) / float(g_ePlayerStats[id][STAT_DEATHS]))
		: float(g_ePlayerStats[id][STAT_KILLS]);

	new len = 0;

	// HTML template using trailing backslash line continuation
	len += formatex(buffer[len], maxlen - len, "\
		<!DOCTYPE html><html><head><meta charset='utf-8'><style>\
		body{background:#111;color:#eee;font-family:Tahoma,sans-serif;font-size:12px;margin:10px;}\
		h3{color:#f39c12;text-align:center;margin:0 0 6px 0;text-transform:uppercase;}\
		.sub{text-align:center;color:#888;font-size:10px;margin-bottom:10px;}\
		.card{background:#1c1c1c;border:1px solid #333;border-radius:4px;padding:8px;margin-bottom:10px;}\
		.title{color:#3498db;font-weight:bold;margin-bottom:4px;font-size:11px;}\
		table{width:100%%;border-collapse:collapse;margin-top:4px;}\
		th{background:#262626;color:#f1c40f;padding:4px;font-size:11px;text-align:left;}\
		td{padding:4px;border-bottom:1px solid #252525;font-size:11px;}\
		.r{text-align:right;}\
		.me{background:#1b3247;color:#fff;font-weight:bold;}\
		</style></head><body>");

	len += formatex(buffer[len], maxlen - len, "\
		<h3>Estatísticas do Mapa - %s</h3>\
		<div class='sub'>Armazenamento em RAM via MemStore (EXP_MAP_END)</div>",
		szCurrentMap
	);

	len += formatex(buffer[len], maxlen - len, "\
		<div class='card'>\
		<div class='title'>Suas Estatísticas (Rank: #%d):</div>\
		Kills: <b>%d</b> | Deaths: <b>%d</b> | K/D: <b>%.2f</b><br>\
		Faca: <b>%d</b> | C4 Plants: <b>%d</b> | C4 Defuses: <b>%d</b>\
		</div>",
		myRank,
		g_ePlayerStats[id][STAT_KILLS],
		g_ePlayerStats[id][STAT_DEATHS],
		flKD,
		g_ePlayerStats[id][STAT_KNIFE_KILLS],
		g_ePlayerStats[id][STAT_C4_PLANTS],
		g_ePlayerStats[id][STAT_C4_DEFUSES]
	);

	len += formatex(buffer[len], maxlen - len, "\
		<table>\
		<tr><th>#</th><th>Jogador</th><th class='r'>K</th><th class='r'>D</th><th class='r'>Faca</th><th class='r'>Plant</th><th class='r'>Defuse</th></tr>");

	new totalRanked = mem_rank_get_count(MAP_STATS_NS);
	new topLimit = (totalRanked > 10) ? 10 : totalRanked;

	new szKey[MAX_AUTHID_LENGTH];
	new szPlayerName[MAX_NAME_LENGTH];
	new pStats[PlayerStats];
	new dummyScore = 0;
	new copied = 0;

	for (new pos = 1; pos <= topLimit; pos++)
	{
		if (!mem_rank_get_top(MAP_STATS_NS, pos, szKey, charsmax(szKey), dummyScore, RANK_DESC))
		{
			break;
		}

		arrayset(pStats, 0, sizeof(pStats));
		mem_get_array(MAP_STATS_NS, szKey, pStats, sizeof(pStats), copied);

		copy(szPlayerName, charsmax(szPlayerName), pStats[STAT_NAME]);
		if (szPlayerName[0] == EOS)
		{
			copy(szPlayerName, charsmax(szPlayerName), szKey);
		}

		replace_all(szPlayerName, charsmax(szPlayerName), "<", "&lt;");
		replace_all(szPlayerName, charsmax(szPlayerName), ">", "&gt;");

		new bool:isSelf = (equal(szKey, g_szAuth[id]) != 0);

		len += formatex(buffer[len], maxlen - len, "\
			<tr class='%s'><td>%d</td><td>%s</td><td class='r'>%d</td><td class='r'>%d</td><td class='r'>%d</td><td class='r'>%d</td><td class='r'>%d</td></tr>",
			isSelf ? "me" : "",
			pos,
			szPlayerName,
			pStats[STAT_KILLS],
			pStats[STAT_DEATHS],
			pStats[STAT_KNIFE_KILLS],
			pStats[STAT_C4_PLANTS],
			pStats[STAT_C4_DEFUSES]
		);
	}

	if (topLimit == 0)
	{
		len += formatex(buffer[len], maxlen - len, "\
			<tr><td colspan='7' style='text-align:center;color:#777;padding:8px;'>Nenhum dado registrado neste mapa ainda.</td></tr>");
	}

	len += formatex(buffer[len], maxlen - len, "</table></body></html>");
}
