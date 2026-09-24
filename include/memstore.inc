// vim: set ts=4 sw=4 tw=99 noet:
//
// AMX Mod X MemStore Include File
// In-Memory Transient Cross-Map Storage for AMX Mod X
//
// Copyright (C) 2026 iceeedR
//
#if defined _memstore_included
	#endinput
#endif
#define _memstore_included

#pragma reqlib memstore
#if !defined AMXMODX_NOAUTOLOAD
	#pragma loadlib memstore
#endif

/**
 * Expiration policies for stored data
 */
enum ExpirePolicy
{
	EXP_PERSISTENT = 0, /**< Retains data in RAM across map changes until server reboot or manual removal */
	EXP_MAP_END    = 1, /**< Automatically cleared on the next map change */
	EXP_MAP_COUNT  = 2, /**< Survives for N map changes (specify N in 'extra' parameter) */
	EXP_TTL        = 3  /**< Expires after N seconds based on wall-clock time (specify seconds in 'extra') */
};

/**
 * Data type stored in a key
 */
enum EntryType
{
	ENTRY_NONE   = 0,
	ENTRY_INT    = 1,
	ENTRY_FLOAT  = 2,
	ENTRY_STRING = 3,
	ENTRY_ARRAY  = 4
};

/**
 * Ranking sort order
 */
enum RankOrder
{
	RANK_DESC = 0, /**< Higher score ranks first (kills, points, score) */
	RANK_ASC  = 1  /**< Lower score ranks first (speedrun time, deaths) */
};

/**
 * Stores an integer cell in memory under the specified namespace and key.
 *
 * @param namespace   Namespace/table name (cannot be empty).
 * @param key         Unique key within the namespace (cannot be empty).
 * @param value       Integer cell value to store.
 * @param policy      Expiration policy (default: EXP_PERSISTENT).
 * @param extra       Extra parameter for policy (e.g., number of maps for EXP_MAP_COUNT or seconds for EXP_TTL).
 *
 * @return            true on success, false if namespace key limit is reached or parameters are invalid.
 */
native bool:mem_set_int(const namespace[], const key[], value, ExpirePolicy:policy = EXP_PERSISTENT, extra = 0);

/**
 * Retrieves an integer cell from memory.
 *
 * @param namespace   Namespace/table name.
 * @param key         Key within the namespace.
 * @param value       Variable to store the retrieved integer cell.
 * @param default_val Value assigned to variable if key doesn't exist or expired (default: 0).
 *
 * @return            true if key was found and valid, false otherwise.
 * @error             Emits an AMXX error if key exists but is not of type Int (Strict Type Checking).
 */
native bool:mem_get_int(const namespace[], const key[], &value, default_val = 0);

/**
 * Stores a float value in memory under the specified namespace and key.
 *
 * @param namespace   Namespace/table name.
 * @param key         Key within the namespace.
 * @param value       Float value to store.
 * @param policy      Expiration policy (default: EXP_PERSISTENT).
 * @param extra       Extra parameter for policy.
 *
 * @return            true on success, false otherwise.
 */
native bool:mem_set_float(const namespace[], const key[], Float:value, ExpirePolicy:policy = EXP_PERSISTENT, extra = 0);

/**
 * Retrieves a float value from memory.
 *
 * @param namespace   Namespace/table name.
 * @param key         Key within the namespace.
 * @param value       Variable to store the retrieved float.
 * @param default_val Value assigned if key doesn't exist or expired (default: 0.0).
 *
 * @return            true if key was found and valid, false otherwise.
 * @error             Emits an AMXX error if key exists but is not of type Float.
 */
native bool:mem_get_float(const namespace[], const key[], &Float:value, Float:default_val = 0.0);

/**
 * Stores a string in memory under the specified namespace and key.
 *
 * @param namespace   Namespace/table name.
 * @param key         Key within the namespace.
 * @param value       String to store.
 * @param policy      Expiration policy (default: EXP_PERSISTENT).
 * @param extra       Extra parameter for policy.
 *
 * @return            true on success, false otherwise.
 */
native bool:mem_set_string(const namespace[], const key[], const value[], ExpirePolicy:policy = EXP_PERSISTENT, extra = 0);

/**
 * Retrieves a string from memory.
 *
 * @param namespace   Namespace/table name.
 * @param key         Key within the namespace.
 * @param dest        Buffer to store retrieved string.
 * @param maxlen      Maximum buffer length.
 * @param default_val Default string copied if key doesn't exist or expired.
 *
 * @return            true if key was found and valid, false otherwise.
 * @error             Emits an AMXX error if key exists but is not of type String.
 */
native bool:mem_get_string(const namespace[], const key[], dest[], maxlen, const default_val[] = "");

/**
 * Stores an array of arbitrary cells in memory under the specified namespace and key.
 *
 * @param namespace   Namespace/table name.
 * @param key         Key within the namespace.
 * @param values      Array of cells to store.
 * @param size        Number of cells to store.
 * @param policy      Expiration policy (default: EXP_PERSISTENT).
 * @param extra       Extra parameter for policy.
 *
 * @return            true on success, false otherwise.
 */
native bool:mem_set_array(const namespace[], const key[], const any:values[], size, ExpirePolicy:policy = EXP_PERSISTENT, extra = 0);

/**
 * Retrieves an array from memory.
 *
 * @param namespace   Namespace/table name.
 * @param key         Key within the namespace.
 * @param dest        Destination array buffer.
 * @param maxlen      Maximum cells to copy.
 * @param copied_size Optional variable to receive the actual number of cells copied.
 *
 * @return            true if key was found and valid, false otherwise.
 * @error             Emits an AMXX error if key exists but is not of type Array.
 */
native bool:mem_get_array(const namespace[], const key[], any:dest[], maxlen, &copied_size = 0);

/**
 * Returns the number of cells stored in an array entry.
 *
 * @param namespace   Namespace/table name.
 * @param key         Key within the namespace.
 *
 * @return            Size of array in cells, or 0 if key does not exist or is not an array.
 */
native mem_get_array_size(const namespace[], const key[]);

/**
 * Returns the data type currently stored under a key.
 *
 * @param namespace   Namespace/table name.
 * @param key         Key within the namespace.
 *
 * @return            EntryType enum value (ENTRY_NONE, ENTRY_INT, ENTRY_FLOAT, ENTRY_STRING, ENTRY_ARRAY).
 */
native EntryType:mem_get_key_type(const namespace[], const key[]);

/**
 * Checks if a key exists in memory and has not expired.
 *
 * @param namespace   Namespace/table name.
 * @param key         Key within the namespace.
 *
 * @return            true if key exists and is valid, false otherwise.
 */
native bool:mem_key_exists(const namespace[], const key[]);

/**
 * Deletes a specific key from a namespace.
 *
 * @param namespace   Namespace/table name.
 * @param key         Key within the namespace.
 *
 * @return            true if key was deleted, false if key did not exist.
 */
native bool:mem_delete_key(const namespace[], const key[]);

/**
 * Deletes all keys within a namespace.
 *
 * @param namespace   Namespace/table name.
 *
 * @return            true if namespace was cleared, false if namespace did not exist.
 */
native bool:mem_clear_namespace(const namespace[]);

/**
 * Updates the expiration policy of an existing key.
 *
 * @param namespace   Namespace/table name.
 * @param key         Key within the namespace.
 * @param policy      New expiration policy.
 * @param extra       Extra parameter for policy.
 *
 * @return            true on success, false if key was not found.
 */
native bool:mem_set_expire(const namespace[], const key[], ExpirePolicy:policy, extra = 0);

/**
 * Returns the number of active, non-expired keys in a namespace.
 *
 * @param namespace   Namespace/table name.
 *
 * @return            Number of active keys, or 0 if namespace is empty/nonexistent.
 */
native mem_get_namespace_count(const namespace[]);

/**
 * Retrieves the key name at a specific 0-based index in a namespace.
 * Useful for iterating through all stored keys without handles or allocations.
 *
 * @param namespace   Namespace/table name.
 * @param index       0-based index (from 0 to mem_get_namespace_count() - 1).
 * @param dest        Buffer to store key name.
 * @param maxlen      Maximum buffer length.
 *
 * @return            true on success, false if index is out of bounds or namespace is empty.
 */
native bool:mem_get_key_at(const namespace[], index, dest[], maxlen);

/**
 * Sets or updates a score for a key in a sorted rank leaderboard.
 *
 * @param namespace   Rank namespace name.
 * @param key         Unique entity/player identifier.
 * @param score       Score integer (kills, points, race time, etc.).
 * @param policy      Expiration policy (default: EXP_PERSISTENT across maps).
 * @param extra       Extra parameter for policy.
 *
 * @return            true on success, false if limit reached.
 */
native bool:mem_rank_set(const namespace[], const key[], score, ExpirePolicy:policy = EXP_PERSISTENT, extra = 0);

/**
 * Retrieves the current score of an entity in a rank namespace.
 *
 * @param namespace   Rank namespace name.
 * @param key         Unique entity/player identifier.
 * @param score       Variable to store score.
 *
 * @return            true if key found and valid, false otherwise.
 */
native bool:mem_rank_get_score(const namespace[], const key[], &score);

/**
 * Returns the 1-based leaderboard position of an entity (1 = 1st place, 2 = 2nd place, etc.).
 * Ties are automatically and deterministically resolved in favor of whoever scored first.
 *
 * @param namespace   Rank namespace name.
 * @param key         Unique entity/player identifier.
 * @param order       Sort order: RANK_DESC (highest score first) or RANK_ASC (lowest score first).
 *
 * @return            1-based rank position (1, 2, 3...), or 0 if key not found in rank.
 */
native mem_rank_get_pos(const namespace[], const key[], RankOrder:order = RANK_DESC);

/**
 * Retrieves the entity key and score at a specific leaderboard position.
 *
 * @param namespace   Rank namespace name.
 * @param rank_pos    1-based position to query (1 = 1st place / champion, 2 = 2nd place, etc.).
 * @param key_dest    Buffer to receive entity key.
 * @param maxlen      Maximum key buffer length.
 * @param score       Optional variable to receive the score.
 * @param order       Sort order (default: RANK_DESC).
 *
 * @return            true if position exists, false if out of range.
 */
native bool:mem_rank_get_top(const namespace[], rank_pos, key_dest[], maxlen, &score = 0, RankOrder:order = RANK_DESC);

/**
 * Returns the total number of ranked entities in a rank namespace.
 *
 * @param namespace   Rank namespace name.
 *
 * @return            Number of entities in the rank.
 */
native mem_rank_get_count(const namespace[]);

/**
 * Deletes an entity from a rank namespace.
 *
 * @param namespace   Rank namespace name.
 * @param key         Entity key to remove.
 *
 * @return            true if deleted, false if not found.
 */
native bool:mem_rank_delete(const namespace[], const key[]);

/**
 * Clears all ranked entities from a rank namespace.
 *
 * @param namespace   Rank namespace name.
 *
 * @return            true if cleared, false if namespace was empty.
 */
native bool:mem_rank_clear(const namespace[]);

/**
 * Configures global memory limits to safeguard against server memory exhaustion.
 *
 * @param max_keys_per_ns Maximum allowed keys per namespace (default: 10000).
 * @param max_array_size  Maximum allowed array elements per key (default: 4096).
 * @param max_string_len  Maximum allowed string characters per key (default: 4096).
 *
 * @return                true on success.
 */
native mem_set_limits(max_keys_per_ns = 10000, max_array_size = 4096, max_string_len = 4096);
