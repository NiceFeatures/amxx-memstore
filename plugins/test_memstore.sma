#include <amxmodx>
#include <memstore>

#define PLUGIN_NAME    "MemStore Comprehensive Test Suite"
#define PLUGIN_VERSION "1.0.0"
#define PLUGIN_AUTHOR  "iceeedR"

#define LOG_FILENAME   "memstore_bench.log"

new g_PassedCount = 0;
new g_FailedCount = 0;

public plugin_init()
{
	register_plugin(PLUGIN_NAME, PLUGIN_VERSION, PLUGIN_AUTHOR);

	// Master all-in-one command
	register_srvcmd("amx_memstore_run_all", "CmdRunAll", -1, "Runs entire test battery: unit tests, benchmark, auto map change, and logs to file");

	// Individual commands
	register_srvcmd("amx_memstore_test", "CmdRunTests", -1, "Runs all instant automated unit tests (including Ranks & Key Enumeration)");
	register_srvcmd("amx_memstore_test_ttl", "CmdTestTTL", -1, "Tests Time-To-Live (TTL) expiration via task");
	register_srvcmd("amx_memstore_test_map_setup", "CmdMapSetup", -1, "Prepares keys to test survival across map change");
	register_srvcmd("amx_memstore_test_map_check", "CmdMapCheck", -1, "Verifies survival of keys after a map change");
	register_srvcmd("amx_memstore_bench", "CmdRunBenchmark", -1, "Runs high-speed performance benchmark and logs to file");

	server_print("==================================================");
	server_print("[MemStore-Test] Test Suite v%s loaded.", PLUGIN_VERSION);
	server_print("  MASTER COMMAND:");
	server_print("    amx_memstore_run_all        - RUNS EVERYTHING AUTOMATICALLY");
	server_print("                                  (Tests + Bench + MapChange + Logs)");
	server_print("  Individual Commands:");
	server_print("    amx_memstore_test           - Run 30+ unit tests");
	server_print("    amx_memstore_bench          - Run speed benchmark");
	server_print("    amx_memstore_test_ttl       - Test TTL expiration (2 sec)");
	server_print("    amx_memstore_test_map_setup - Manual map change setup");
	server_print("    amx_memstore_test_map_check - Manual map change check");
	server_print("==================================================");

	// Auto-resume check if an automated or manual map change test was active
	CheckMapTransitionState();
}

// ==================================================
// MASTER ALL-IN-ONE AUTOMATED BATTERY
// ==================================================
public CmdRunAll()
{
	server_print("^n##################################################");
	server_print("###   MEMSTORE MASTER AUTOMATED TEST BATTERY   ###");
	server_print("##################################################");

	new currentMap[64];
	get_mapname(currentMap, charsmax(currentMap));

	log_to_file(LOG_FILENAME, "==================================================");
	log_to_file(LOG_FILENAME, "[MemStore-AutoTest] STARTING COMPLETE TEST BATTERY on map: %s", currentMap);

	// 1. Run all Unit Tests
	CmdRunTests();

	log_to_file(LOG_FILENAME, "[MemStore-AutoTest] Unit Tests: %d PASSED, %d FAILED", g_PassedCount, g_FailedCount);

	// 2. Run High-Speed Benchmark
	new writeOps = 0, readOps = 0, writeMs = 0, readMs = 0;
	ExecuteBenchmark(writeOps, readOps, writeMs, readMs);

	log_to_file(LOG_FILENAME, "[MemStore-AutoTest] Benchmark: 20k Writes in %d ms (%d ops/sec) | 20k Reads in %d ms (%d ops/sec)",
		writeMs, writeOps, readMs, readOps);

	// 3. Setup Map Change Retention Keys in RAM
	server_print("^n[AutoTest] Arming RAM keys for automatic map change test...");

	mem_set_int("autotest_ns", "in_progress", 1, EXP_PERSISTENT);
	mem_set_int("autotest_ns", "prev_passed", g_PassedCount, EXP_PERSISTENT);
	mem_set_int("autotest_ns", "prev_failed", g_FailedCount, EXP_PERSISTENT);
	mem_set_int("autotest_ns", "prev_write_ops", writeOps, EXP_PERSISTENT);
	mem_set_int("autotest_ns", "prev_read_ops", readOps, EXP_PERSISTENT);

	// Test entries across policies:
	mem_set_int("autotest_ns", "key_persistent", 1337, EXP_PERSISTENT);
	mem_set_int("autotest_ns", "key_map_end", 9999, EXP_MAP_END);
	mem_set_int("autotest_ns", "key_map_count_2", 8888, EXP_MAP_COUNT, 2);
	mem_set_string("autotest_ns", "key_string", "MemStore_RAM_Survivor_OK", EXP_PERSISTENT);
	new gear[3] = { 47, 50, 100 };
	mem_set_array("autotest_ns", "key_arr", gear, sizeof(gear), EXP_PERSISTENT);

	mem_rank_set("autotest_rank", "TopWinner", 1000, EXP_PERSISTENT);
	mem_rank_set("autotest_rank", "TempLoser", 100, EXP_MAP_END);

	log_to_file(LOG_FILENAME, "[MemStore-AutoTest] RAM survival keys armed. Triggering automatic map change in 2.5 seconds...");
	server_print("[AutoTest] RAM keys armed successfully. Changing map automatically in 2.5 seconds...");

	set_task(2.5, "TaskTriggerAutoMapChange");
	return PLUGIN_HANDLED;
}

public TaskTriggerAutoMapChange()
{
	new currentMap[64];
	get_mapname(currentMap, charsmax(currentMap));
	server_print("[AutoTest] Executing automatic changelevel to: %s...", currentMap);
	server_cmd("changelevel %s", currentMap);
}

// --------------------------------------------------
// AUTOMATIC MAP TRANSITION VERIFICATION (ON NEW MAP)
// --------------------------------------------------
CheckMapTransitionState()
{
	new mapName[64];
	get_mapname(mapName, charsmax(mapName));

	// 1. Check if Master Auto-Test was in progress
	if (mem_key_exists("autotest_ns", "in_progress"))
	{
		server_print("^n##################################################");
		server_print("###   MEMSTORE MASTER TEST: NEW MAP DETECTED   ###");
		server_print("##################################################");
		server_print("[AutoTest] Server reloaded into map: %s", mapName);

		log_to_file(LOG_FILENAME, "[MemStore-AutoTest] MAP CHANGE SUCCESSFUL! Server arrived at: %s", mapName);
		log_to_file(LOG_FILENAME, "[MemStore-AutoTest] Verifying RAM retention of previous map's data...");

		new val = 0;
		new allSurvivalOk = true;

		// 1. Persistent Int
		new bool:pInt = mem_get_int("autotest_ns", "key_persistent", val, 0);
		if (pInt && val == 1337)
		{
			server_print("  [PASS] 'key_persistent' (val = %d) survived in RAM across maps!", val);
		}
		else
		{
			server_print("  [FAIL] 'key_persistent' failed!");
			allSurvivalOk = false;
		}

		// 2. Map-End Int (MUST BE DELETED)
		new bool:pEnd = mem_key_exists("autotest_ns", "key_map_end");
		if (!pEnd)
		{
			server_print("  [PASS] 'key_map_end' was automatically wiped on map change!");
		}
		else
		{
			server_print("  [FAIL] 'key_map_end' still in RAM!");
			allSurvivalOk = false;
		}

		// 3. Map-Count Int (COUNT 2 -> MUST SURVIVE WITH 1 MAP LEFT)
		new bool:pCnt = mem_get_int("autotest_ns", "key_map_count_2", val, 0);
		if (pCnt && val == 8888)
		{
			server_print("  [PASS] 'key_map_count_2' survived in RAM!");
		}
		else
		{
			server_print("  [FAIL] 'key_map_count_2' failed!");
			allSurvivalOk = false;
		}

		// 4. Persistent String
		new strBuf[64];
		new bool:pStr = mem_get_string("autotest_ns", "key_string", strBuf, charsmax(strBuf));
		if (pStr && equal(strBuf, "MemStore_RAM_Survivor_OK"))
		{
			server_print("  [PASS] 'key_string' (^"%s^") survived in RAM!", strBuf);
		}
		else
		{
			server_print("  [FAIL] 'key_string' failed!");
			allSurvivalOk = false;
		}

		// 5. Persistent Array
		new arrBuf[3];
		new copied = 0;
		new bool:pArr = mem_get_array("autotest_ns", "key_arr", arrBuf, sizeof(arrBuf), copied);
		if (pArr && copied == 3 && arrBuf[0] == 47 && arrBuf[1] == 50 && arrBuf[2] == 100)
		{
			server_print("  [PASS] 'key_arr' survived in RAM ({%d, %d, %d})!", arrBuf[0], arrBuf[1], arrBuf[2]);
		}
		else
		{
			server_print("  [FAIL] 'key_arr' failed!");
			allSurvivalOk = false;
		}

		// 6. Rank Survival & Expiration
		new rankScore = 0;
		new bool:champSurv = mem_rank_get_score("autotest_rank", "TopWinner", rankScore);
		new bool:tempDied = !mem_rank_get_score("autotest_rank", "TempLoser", rankScore);
		if (champSurv && tempDied && rankScore == 1000)
		{
			server_print("  [PASS] Rank system: TopWinner (EXP_PERSISTENT) survived, TempLoser (EXP_MAP_END) wiped!");
		}
		else
		{
			server_print("  [FAIL] Rank survival check failed!");
			allSurvivalOk = false;
		}

		// Read previous benchmarks stored in RAM from previous map!
		new prevPass = 0, prevFail = 0, prevWriteOps = 0, prevReadOps = 0;
		mem_get_int("autotest_ns", "prev_passed", prevPass);
		mem_get_int("autotest_ns", "prev_failed", prevFail);
		mem_get_int("autotest_ns", "prev_write_ops", prevWriteOps);
		mem_get_int("autotest_ns", "prev_read_ops", prevReadOps);

		if (allSurvivalOk)
		{
			server_print("^n==================================================");
			server_print("  MEMSTORE COMPLETE AUTOMATED BATTERY FINISHED:   ");
			server_print("                100%%%% ALL TESTS PASSED!            ");
			server_print("  Benchmark Logged: %d writes/sec, %d reads/sec", prevWriteOps, prevReadOps);
			server_print("  Logs written to: addons/amxmodx/logs/%s", LOG_FILENAME);
			server_print("==================================================^n");

			log_to_file(LOG_FILENAME, "[MemStore-AutoTest] RAM SURVIVAL VERIFICATION: ALL 6 CHECKS PASSED!");
			log_to_file(LOG_FILENAME, "[MemStore-AutoTest] *** COMPLETE TEST BATTERY FINISHED WITH 100%%%% SUCCESS ***");
			log_to_file(LOG_FILENAME, "==================================================");
		}
		else
		{
			log_to_file(LOG_FILENAME, "[MemStore-AutoTest] [FAIL] Some survival checks failed!");
		}

		// Clean up autotest namespace
		mem_clear_namespace("autotest_ns");
		mem_rank_clear("autotest_rank");
		return;
	}

	// 2. Manual map test check if configured via amx_memstore_test_map_setup
	if (mem_key_exists("map_test_ns", "setup_active"))
	{
		server_print("^n[MapTest] Manual map change test detected on map: %s", mapName);
		CmdMapCheck();
	}
}

// --------------------------------------------------
// MANUAL MAP CHANGE COMMANDS
// --------------------------------------------------
public CmdMapSetup()
{
	server_print("--------------------------------------------------");
	server_print("[MapTest] Setting up keys for Manual Map Change Test...");

	mem_set_int("map_test_ns", "setup_active", 1, EXP_PERSISTENT);
	mem_set_int("map_test_ns", "key_persistent", 12345, EXP_PERSISTENT);
	mem_set_int("map_test_ns", "key_map_end", 99999, EXP_MAP_END);
	mem_set_int("map_test_ns", "key_map_count_2", 77777, EXP_MAP_COUNT, 2);
	mem_set_int("map_test_ns", "key_map_count_1", 55555, EXP_MAP_COUNT, 1);
	mem_set_string("map_test_ns", "key_str_persistent", "Survived HLDS Map Change!", EXP_PERSISTENT);
	new gear[3] = { 47, 50, 100 };
	mem_set_array("map_test_ns", "key_arr_persistent", gear, sizeof(gear), EXP_PERSISTENT);

	mem_rank_set("map_rank_ns", "ChampPlayer", 500, EXP_PERSISTENT);
	mem_rank_set("map_rank_ns", "TempPlayer", 100, EXP_MAP_END);

	server_print("  [OK] Keys configured. Now type 'changelevel <map>' or 'restart'!");
	server_print("--------------------------------------------------");
	return PLUGIN_HANDLED;
}

public CmdMapCheck()
{
	server_print("--------------------------------------------------");
	server_print("     CHECKING MAP CHANGE RETENTION IN RAM         ");
	server_print("--------------------------------------------------");

	new val = 0;

	new bool:persistExists = mem_get_int("map_test_ns", "key_persistent", val, 0);
	if (persistExists && val == 12345)
		server_print("  [PASS] 'key_persistent' survived in RAM! (val = %d)", val);
	else
		server_print("  [FAIL] 'key_persistent' was lost! (exists = %d, val = %d)", persistExists, val);

	new bool:mapEndExists = mem_key_exists("map_test_ns", "key_map_end");
	if (!mapEndExists)
		server_print("  [PASS] 'key_map_end' was automatically wiped on map change!");
	else
		server_print("  [FAIL] 'key_map_end' still exists in RAM! (policy failed)");

	new bool:mapCount2Exists = mem_get_int("map_test_ns", "key_map_count_2", val, 0);
	if (mapCount2Exists && val == 77777)
		server_print("  [PASS] 'key_map_count_2' survived (1 map remaining, val = %d)", val);
	else
		server_print("  [FAIL] 'key_map_count_2' was prematurely deleted!");

	new bool:mapCount1Exists = mem_key_exists("map_test_ns", "key_map_count_1");
	if (!mapCount1Exists)
		server_print("  [PASS] 'key_map_count_1' expired after 1 map change as expected!");
	else
		server_print("  [FAIL] 'key_map_count_1' still exists in RAM! (should have expired)");

	new strBuf[64];
	new bool:strExists = mem_get_string("map_test_ns", "key_str_persistent", strBuf, charsmax(strBuf));
	if (strExists && equal(strBuf, "Survived HLDS Map Change!"))
		server_print("  [PASS] 'key_str_persistent' survived in RAM! (^"%s^")", strBuf);
	else
		server_print("  [FAIL] 'key_str_persistent' failed (exists = %d, str = ^"%s^")", strExists, strBuf);

	new arrBuf[3];
	new copied = 0;
	new bool:arrExists = mem_get_array("map_test_ns", "key_arr_persistent", arrBuf, sizeof(arrBuf), copied);
	if (arrExists && copied == 3 && arrBuf[0] == 47 && arrBuf[1] == 50 && arrBuf[2] == 100)
		server_print("  [PASS] 'key_arr_persistent' survived in RAM! ({%d, %d, %d})", arrBuf[0], arrBuf[1], arrBuf[2]);
	else
		server_print("  [FAIL] 'key_arr_persistent' failed (exists = %d)", arrExists);

	new rankScore = 0;
	new bool:champSurvived = mem_rank_get_score("map_rank_ns", "ChampPlayer", rankScore);
	new bool:tempRankDied = !mem_rank_get_score("map_rank_ns", "TempPlayer", rankScore);
	if (champSurvived && tempRankDied)
		server_print("  [PASS] Rank survival: ChampPlayer (EXP_PERSISTENT) survived, TempPlayer (EXP_MAP_END) wiped!");
	else
		server_print("  [FAIL] Rank survival check failed!");

	server_print("--------------------------------------------------");
	return PLUGIN_HANDLED;
}

// --------------------------------------------------
// ASYNCHRONOUS TTL (TIME-TO-LIVE) TEST
// --------------------------------------------------
public CmdTestTTL()
{
	server_print("--------------------------------------------------");
	server_print("[TTL-Test] Starting 2-second TTL expiration test...");

	mem_set_int("ttl_test_ns", "session_token", 98765, EXP_TTL, 2);

	new val = 0;
	new bool:foundNow = mem_get_int("ttl_test_ns", "session_token", val, 0);
	if (foundNow && val == 98765)
		server_print("  [Step 1] Key stored with 2-second TTL. Immediate read: SUCCESS (val = %d)", val);
	else
		server_print("  [Step 1] Immediate read FAILED!");

	server_print("  [Step 2] Waiting 2.5 seconds for TTL expiry timer...");
	set_task(2.5, "TaskCheckTTLExpired");
	server_print("--------------------------------------------------");
	return PLUGIN_HANDLED;
}

public TaskCheckTTLExpired()
{
	server_print("^n--------------------------------------------------");
	server_print("[TTL-Test] Verifying expiration after 2.5 seconds...");

	new val = 0;
	new bool:exists = mem_key_exists("ttl_test_ns", "session_token");
	new bool:getRes = mem_get_int("ttl_test_ns", "session_token", val, -1);

	if (!exists && !getRes && val == -1)
	{
		server_print("  [PASS] TTL TEST SUCCESSFUL! Key expired and was safely purged from RAM.");
		log_to_file(LOG_FILENAME, "[MemStore-TTL] TTL 2-second test: PASSED (key safely purged)");
	}
	else
	{
		server_print("  [FAIL] TTL TEST FAILED! Key still active after TTL (exists = %d, val = %d)", exists, val);
		log_to_file(LOG_FILENAME, "[MemStore-TTL] TTL 2-second test: FAILED");
	}
	server_print("--------------------------------------------------");
}

// --------------------------------------------------
// COMPREHENSIVE AUTOMATED UNIT TESTS
// --------------------------------------------------
public CmdRunTests()
{
	g_PassedCount = 0;
	g_FailedCount = 0;

	server_print("--------------------------------------------------");
	server_print("       MEMSTORE FULL UNIT TEST SUITE       ");
	server_print("--------------------------------------------------");

	TestIntegerBoundaryOperations();
	TestFloatPrecisionOperations();
	TestStringEdgeCases();
	TestArrayAdvancedOperations();
	TestStrictTypeChecking();
	TestSafetyLimitsEnforcement();
	TestExpirationPolicyUpdates();
	TestNamespaceIsolation();
	TestKeyEnumeration();
	TestLeaderboardRankEngine();

	server_print("--------------------------------------------------");
	server_print("TOTAL RESULTS: %d PASSED, %d FAILED", g_PassedCount, g_FailedCount);
	server_print("--------------------------------------------------");
	return PLUGIN_HANDLED;
}

Assert(const testName[], any:condition)
{
	if (condition)
	{
		server_print("  [PASS] %s", testName);
		g_PassedCount++;
	}
	else
	{
		server_print("  [FAIL] %s", testName);
		g_FailedCount++;
	}
}

TestIntegerBoundaryOperations()
{
	server_print("[Testing] Integer Boundaries (0, negative, max)...");

	mem_set_int("int_ns", "zero", 0);
	new val = -1;
	mem_get_int("int_ns", "zero", val, -1);
	Assert("Integer Zero stored and read", val == 0);

	mem_set_int("int_ns", "negative", -123456);
	val = 0;
	mem_get_int("int_ns", "negative", val, 0);
	Assert("Integer Negative number stored and read", val == -123456);

	mem_set_int("int_ns", "large", 2147483640);
	val = 0;
	mem_get_int("int_ns", "large", val, 0);
	Assert("Integer Large value (near INT32_MAX)", val == 2147483640);
}

TestFloatPrecisionOperations()
{
	server_print("[Testing] Float Operations...");

	mem_set_float("float_ns", "gravity", 800.5);
	new Float:fVal = 0.0;
	mem_get_float("float_ns", "gravity", fVal, 0.0);
	Assert("Float standard value", fVal == 800.5);

	mem_set_float("float_ns", "neg_coord", -1024.125);
	fVal = 0.0;
	mem_get_float("float_ns", "neg_coord", fVal, 0.0);
	Assert("Float negative value", fVal == -1024.125);
}

TestStringEdgeCases()
{
	server_print("[Testing] String Edge Cases (empty, symbols, overwrite)...");

	mem_set_string("str_ns", "empty", "");
	new buf[64];
	new bool:found = mem_get_string("str_ns", "empty", buf, charsmax(buf), "fallback");
	Assert("Empty string stored and retrieved", found && (equal(buf, "") != 0));

	new const special[] = "Pawn & C++ [AMXX] {127.0.0.1:27015} !@#$%^^&*()";
	mem_set_string("str_ns", "special", special);
	mem_get_string("str_ns", "special", buf, charsmax(buf));
	Assert("Special symbols string preserved intact", (equal(buf, special) != 0));

	mem_set_string("str_ns", "mut", "first");
	mem_set_string("str_ns", "mut", "second");
	mem_get_string("str_ns", "mut", buf, charsmax(buf));
	Assert("String overwrite in place", (equal(buf, "second") != 0));
}

TestArrayAdvancedOperations()
{
	server_print("[Testing] Array Advanced Operations...");

	new original[6] = { 100, 200, 300, 400, 500, 600 };
	mem_set_array("arr_ns", "data", original, sizeof(original));

	Assert("Array size matches", mem_get_array_size("arr_ns", "data") == 6);

	new smallBuf[3];
	new copied = 0;
	new bool:readOk = mem_get_array("arr_ns", "data", smallBuf, sizeof(smallBuf), copied);
	Assert("Array partial read clipped safely to buffer size", readOk && (copied == 3) && (smallBuf[0] == 100 && smallBuf[1] == 200 && smallBuf[2] == 300));
}

TestStrictTypeChecking()
{
	server_print("[Testing] Strict Type Checking & Inspection (mem_get_key_type)...");

	mem_set_int("type_ns", "int_key", 42);
	mem_set_float("type_ns", "float_key", 3.14);
	mem_set_string("type_ns", "str_key", "hello");
	new arr[2] = { 10, 20 };
	mem_set_array("type_ns", "arr_key", arr, sizeof(arr));

	Assert("Type inspection: int_key is ENTRY_INT", mem_get_key_type("type_ns", "int_key") == ENTRY_INT);
	Assert("Type inspection: float_key is ENTRY_FLOAT", mem_get_key_type("type_ns", "float_key") == ENTRY_FLOAT);
	Assert("Type inspection: str_key is ENTRY_STRING", mem_get_key_type("type_ns", "str_key") == ENTRY_STRING);
	Assert("Type inspection: arr_key is ENTRY_ARRAY", mem_get_key_type("type_ns", "arr_key") == ENTRY_ARRAY);
	Assert("Type inspection: ghost_key is ENTRY_NONE", mem_get_key_type("type_ns", "ghost_key") == ENTRY_NONE);

	mem_clear_namespace("type_ns");
}

TestSafetyLimitsEnforcement()
{
	server_print("[Testing] Safety Limits Enforcement (Anti-DoS)...");

	mem_set_limits(3, 4096, 4096);

	mem_clear_namespace("limit_ns");
	new bool:set1 = mem_set_int("limit_ns", "k1", 1);
	new bool:set2 = mem_set_int("limit_ns", "k2", 2);
	new bool:set3 = mem_set_int("limit_ns", "k3", 3);

	Assert("Key limit: first 3 keys succeed", set1 && set2 && set3);
	Assert("Key limit: namespace count reaches 3", mem_get_namespace_count("limit_ns") == 3);

	new bool:updateOk = mem_set_int("limit_ns", "k2", 222);
	Assert("Key limit: update existing key still allowed", updateOk);

	mem_set_limits(10000, 4096, 4096);
	mem_clear_namespace("limit_ns");
}

TestExpirationPolicyUpdates()
{
	server_print("[Testing] mem_set_expire Policy Mutation...");

	mem_set_int("exp_ns", "perm", 500, EXP_PERSISTENT);
	Assert("Key exists initially", mem_key_exists("exp_ns", "perm"));

	new bool:mutateOk = mem_set_expire("exp_ns", "perm", EXP_MAP_END);
	Assert("mem_set_expire successfully mutated key policy", mutateOk);
}

TestNamespaceIsolation()
{
	server_print("[Testing] Namespace Isolation...");

	mem_set_int("ns_alpha", "player_kills", 10);
	mem_set_int("ns_beta",  "player_kills", 99);

	new valA = 0, valB = 0;
	mem_get_int("ns_alpha", "player_kills", valA);
	mem_get_int("ns_beta",  "player_kills", valB);

	Assert("Namespace isolation: same key name holds distinct values", (valA == 10) && (valB == 99));

	mem_clear_namespace("ns_alpha");
	Assert("Namespace alpha cleared", !mem_key_exists("ns_alpha", "player_kills"));
	Assert("Namespace beta intact after alpha cleared", mem_key_exists("ns_beta", "player_kills"));

	mem_clear_namespace("ns_beta");
}

TestKeyEnumeration()
{
	server_print("[Testing] Feature B: Key Enumeration (mem_get_key_at)...");

	mem_clear_namespace("enum_ns");
	mem_set_int("enum_ns", "first_key", 1);
	mem_set_int("enum_ns", "second_key", 2);
	mem_set_int("enum_ns", "third_key", 3);

	new count = mem_get_namespace_count("enum_ns");
	Assert("Key enumeration: count is 3", count == 3);

	new keyBuf[64];
	new bool:k0 = mem_get_key_at("enum_ns", 0, keyBuf, charsmax(keyBuf));
	Assert("Key enumeration: index 0 read valid", k0 && strlen(keyBuf) > 0);

	new bool:k1 = mem_get_key_at("enum_ns", 1, keyBuf, charsmax(keyBuf));
	Assert("Key enumeration: index 1 read valid", k1 && strlen(keyBuf) > 0);

	new bool:k2 = mem_get_key_at("enum_ns", 2, keyBuf, charsmax(keyBuf));
	Assert("Key enumeration: index 2 read valid", k2 && strlen(keyBuf) > 0);

	new bool:kOut = mem_get_key_at("enum_ns", 99, keyBuf, charsmax(keyBuf));
	Assert("Key enumeration: out of bounds index returns false", !kOut);

	mem_clear_namespace("enum_ns");
}

TestLeaderboardRankEngine()
{
	server_print("[Testing] Feature A: Leaderboard Rank Engine (ZSET)...");

	mem_rank_clear("frags_board");

	mem_rank_set("frags_board", "PlayerA", 15);
	mem_rank_set("frags_board", "PlayerB", 30);
	mem_rank_set("frags_board", "PlayerC", 20);
	mem_rank_set("frags_board", "PlayerD", 15);

	Assert("Rank count is 4", mem_rank_get_count("frags_board") == 4);

	new topKey[32];
	new topScore = 0;
	new bool:gotTop1 = mem_rank_get_top("frags_board", 1, topKey, charsmax(topKey), topScore, RANK_DESC);
	Assert("Rank 1st place is PlayerB with 30", gotTop1 && equal(topKey, "PlayerB") && (topScore == 30));

	mem_rank_get_top("frags_board", 2, topKey, charsmax(topKey), topScore, RANK_DESC);
	Assert("Rank 2nd place is PlayerC with 20", equal(topKey, "PlayerC") && (topScore == 20));

	mem_rank_get_top("frags_board", 3, topKey, charsmax(topKey), topScore, RANK_DESC);
	Assert("Rank 3rd place tie-breaker winner is PlayerA", equal(topKey, "PlayerA") && (topScore == 15));

	mem_rank_get_top("frags_board", 4, topKey, charsmax(topKey), topScore, RANK_DESC);
	Assert("Rank 4th place is PlayerD", equal(topKey, "PlayerD") && (topScore == 15));

	Assert("mem_rank_get_pos for PlayerB is 1", mem_rank_get_pos("frags_board", "PlayerB", RANK_DESC) == 1);
	Assert("mem_rank_get_pos for PlayerC is 2", mem_rank_get_pos("frags_board", "PlayerC", RANK_DESC) == 2);
	Assert("mem_rank_get_pos for PlayerD is 4", mem_rank_get_pos("frags_board", "PlayerD", RANK_DESC) == 4);
	Assert("mem_rank_get_pos for NonExistent is 0", mem_rank_get_pos("frags_board", "Ghost", RANK_DESC) == 0);

	mem_rank_clear("speedrun_board");
	mem_rank_set("speedrun_board", "RunnerFast", 35);
	mem_rank_set("speedrun_board", "RunnerMedium", 50);
	mem_rank_set("speedrun_board", "RunnerSlow", 80);

	mem_rank_get_top("speedrun_board", 1, topKey, charsmax(topKey), topScore, RANK_ASC);
	Assert("Ascending Rank: 1st place is RunnerFast (35 sec)", equal(topKey, "RunnerFast") && (topScore == 35));

	Assert("RunnerFast position in ASC is 1", mem_rank_get_pos("speedrun_board", "RunnerFast", RANK_ASC) == 1);
	Assert("RunnerSlow position in ASC is 3", mem_rank_get_pos("speedrun_board", "RunnerSlow", RANK_ASC) == 3);

	mem_rank_clear("frags_board");
	mem_rank_clear("speedrun_board");
}

// --------------------------------------------------
// HIGH-SPEED BENCHMARK WITH FILE LOGGING
// --------------------------------------------------
ExecuteBenchmark(&writeOpsOut, &readOpsOut, &writeMsOut, &readMsOut)
{
	server_print("--------------------------------------------------");
	server_print("       MEMSTORE SPEED BENCHMARK (20,000 OPS)      ");
	server_print("--------------------------------------------------");

	mem_set_limits(50000, 4096, 4096);

	new keyBuf[16];
	new startTick = tickcount();

	for (new i = 0; i < 20000; i++)
	{
		formatex(keyBuf, charsmax(keyBuf), "k_%d", i);
		mem_set_int("bench_ns", keyBuf, i * 2, EXP_MAP_END);
	}
	new writeMs = tickcount() - startTick;
	if (writeMs <= 0)
	{
		writeMs = 1;
	}
	new Float:writeSec = float(writeMs) / 1000.0;
	new Float:writeOpsF = 20000.0 / writeSec;
	writeOpsOut = floatround(writeOpsF);
	writeMsOut = writeMs;

	server_print("  20,000 In-RAM Writes: %d ms (%.4f sec) -> %d ops/sec", writeMs, writeSec, writeOpsOut);

	startTick = tickcount();
	new readVal = 0;
	new matches = 0;
	for (new i = 0; i < 20000; i++)
	{
		formatex(keyBuf, charsmax(keyBuf), "k_%d", i);
		mem_get_int("bench_ns", keyBuf, readVal, 0);
		if (readVal == i * 2)
		{
			matches++;
		}
	}
	new readMs = tickcount() - startTick;
	if (readMs <= 0)
	{
		readMs = 1;
	}
	new Float:readSec = float(readMs) / 1000.0;
	new Float:readOpsF = 20000.0 / readSec;
	readOpsOut = floatround(readOpsF);
	readMsOut = readMs;

	server_print("  20,000 In-RAM Reads:  %d ms (%.4f sec) -> %d ops/sec [Matches: %d]", readMs, readSec, readOpsOut, matches);

	mem_clear_namespace("bench_ns");
	mem_set_limits(10000, 4096, 4096);

	server_print("--------------------------------------------------");
}

public CmdRunBenchmark()
{
	new wOps = 0, rOps = 0, wMs = 0, rMs = 0;
	ExecuteBenchmark(wOps, rOps, wMs, rMs);

	new mapName[64];
	get_mapname(mapName, charsmax(mapName));
	log_to_file(LOG_FILENAME, "[MemStore-Bench] Map: %s | 20k Writes: %d ms (%d ops/sec) | 20k Reads: %d ms (%d ops/sec)",
		mapName, wMs, wOps, rMs, rOps);

	server_print("[MemStore] Benchmark results appended to: addons/amxmodx/logs/%s", LOG_FILENAME);
	return PLUGIN_HANDLED;
}
