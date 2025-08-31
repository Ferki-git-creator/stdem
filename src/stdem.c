/**
 * @file stdem.c
 * @brief Standard Enum Mapping Library - Implementation
 * @author Ferki
 * @license LGPL-3.0-or-later
 * 
 * @copyright Copyright (c) 2023 Ferki. Permission is granted to copy, distribute
 * and/or modify this document under the terms of the GNU Lesser General Public
 * License, Version 3 or any later version published by the Free Software Foundation.
 * 
 * @details
 * This implementation provides a highly efficient, platform-independent enum mapping
 * system with the following features:
 * - Hash table with separate chaining for collision resolution
 * - Automatic resizing based on load factor
 * - Support for both value copying and pointer storage
 * - Comprehensive error handling and reporting
 * - Memory efficiency with minimal overhead
 * - Complete platform independence without any conditional compilation
 */

#include "stdem.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ==================== INTERNAL STRUCTURES ==================== */

/**
 * @brief Internal structure representing a single entry in the enum map
 * 
 * This structure holds the enum value, its associated name (if any),
 * the value pointer or copied value, and a pointer to the next entry
 * in the bucket for collision resolution.
 */
typedef struct EnumEntry {
    int enum_value;         /**< The integer value of the enum entry */
    char* name;             /**< The name of the enum entry (optional) */
    void* value;            /**< Pointer to the associated value */
    struct EnumEntry* next; /**< Next entry in the bucket (for chaining) */
} EnumEntry;

/**
 * @brief Internal structure representing the enum map
 * 
 * This structure holds all the metadata and data for the enum map,
 * including the number of entries, value size, flags, and the hash table buckets.
 */
struct EnumMap {
    size_t count;           /**< Number of entries in the map */
    size_t value_size;      /**< Size of each value in bytes (0 for pointer storage) */
    StemFlags flags;        /**< Configuration flags */
    EnumEntry** buckets;    /**< Array of buckets for the hash table */
    size_t num_buckets;     /**< Number of buckets in the hash table */
    
    /**
     * @brief Mutex for thread safety
     * 
     * This pointer can be used to store a platform-specific mutex handle.
     * For maximum portability, the library doesn't implement thread safety
     * by default, but provides hooks for external synchronization.
     */
    void* mutex;
};

/* ==================== INTERNAL CONSTANTS ==================== */

#define STEM_DEFAULT_BUCKETS 16    /**< Default number of buckets in the hash table */
#define STEM_LOAD_FACTOR 0.75      /**< Load factor threshold for resizing the hash table */
#define STEM_MAX_ENTRIES ((size_t)-1) /**< Maximum number of entries supported */

/* ==================== INTERNAL FUNCTION PROTOTYPES ==================== */

static uint32_t stem_hash_int(int value);
static uint32_t stem_hash_string(const char* str);
static EnumEntry* stem_find_entry(const EnumMap* map, int enum_value);
static StemError stem_resize_map(EnumMap* map, size_t new_size);
static StemError stem_create_entry(EnumMap* map, int enum_value, 
                                 const void* value, const char* name, 
                                 EnumEntry* *out_entry);
static void stem_free_entry(EnumMap* map, EnumEntry* entry);
static void stem_lock_map(EnumMap* map);
static void stem_unlock_map(EnumMap* map);

/* ==================== THREAD SAFETY STUBS ==================== */

/**
 * @brief Locks the enum map for thread-safe operations
 * 
 * This is a stub function that does nothing by default.
 * For actual thread safety, this should be implemented with
 * platform-specific synchronization primitives.
 * 
 * @param map Pointer to the EnumMap to lock
 */
static void stem_lock_map(EnumMap* map) {
    /* Default implementation: no-op
     * For thread safety, replace with platform-specific locking
     */
    (void)map; /* Unused parameter */
}

/**
 * @brief Unlocks the enum map after thread-safe operations
 * 
 * This is a stub function that does nothing by default.
 * For actual thread safety, this should be implemented with
 * platform-specific synchronization primitives.
 * 
 * @param map Pointer to the EnumMap to unlock
 */
static void stem_unlock_map(EnumMap* map) {
    /* Default implementation: no-op
     * For thread safety, replace with platform-specific locking
     */
    (void)map; /* Unused parameter */
}

/* ==================== HASHING FUNCTIONS ==================== */

/**
 * @brief Hashes an integer value for bucket indexing
 * 
 * Uses a simple multiplication hash that provides good distribution
 * for typical enum values.
 * 
 * @param value The integer value to hash
 * @return uint32_t The hash value
 */
static uint32_t stem_hash_int(int value) {
    /* Multiplication by a large prime for good distribution */
    return (uint32_t)value * 2654435761U;
}

/**
 * @brief Hashes a string for bucket indexing (used for name lookup)
 * 
 * This function uses the djb2 algorithm for string hashing, which provides
 * good distribution and is efficient for typical enum name lengths.
 * 
 * @param str The string to hash
 * @return uint32_t The hash value
 */
static uint32_t stem_hash_string(const char* str) {
    uint32_t hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}

/* ==================== INTERNAL MAP OPERATIONS ==================== */

/**
 * @brief Finds an entry in the map by enum value
 * 
 * This function searches the hash table for an entry with the given enum value.
 * It handles collisions by traversing the linked list in the appropriate bucket.
 * 
 * @param map Pointer to the EnumMap
 * @param enum_value The enum value to search for
 * @return EnumEntry* Pointer to the found entry, or NULL if not found
 */
static EnumEntry* stem_find_entry(const EnumMap* map, int enum_value) {
    if (!map || !map->buckets) {
        return NULL;
    }
    
    uint32_t hash = stem_hash_int(enum_value);
    size_t bucket_idx = hash % map->num_buckets;
    
    EnumEntry* entry = map->buckets[bucket_idx];
    while (entry) {
        if (entry->enum_value == enum_value) {
            return entry;
        }
        entry = entry->next;
    }
    
    return NULL;
}

/**
 * @brief Resizes the hash table to a new size
 * 
 * This function rehashes all entries and places them into new buckets.
 * It's called automatically when the load factor exceeds STEM_LOAD_FACTOR.
 * 
 * @param map Pointer to the EnumMap
 * @param new_size The new number of buckets
 * @return StemError Error code indicating success or failure
 */
static StemError stem_resize_map(EnumMap* map, size_t new_size) {
    if (!map) {
        return STEM_ERROR_INVALID_ARG;
    }
    
    if (new_size == 0) {
        return STEM_ERROR_INVALID_ARG;
    }
    
    EnumEntry** new_buckets = calloc(new_size, sizeof(EnumEntry*));
    if (!new_buckets) {
        return STEM_ERROR_OUT_OF_MEMORY;
    }
    
    /* Rehash all entries */
    for (size_t i = 0; i < map->num_buckets; i++) {
        EnumEntry* entry = map->buckets[i];
        while (entry) {
            EnumEntry* next = entry->next;
            
            uint32_t hash = stem_hash_int(entry->enum_value);
            size_t new_bucket_idx = hash % new_size;
            
            entry->next = new_buckets[new_bucket_idx];
            new_buckets[new_bucket_idx] = entry;
            
            entry = next;
        }
    }
    
    free(map->buckets);
    map->buckets = new_buckets;
    map->num_buckets = new_size;
    
    return STEM_SUCCESS;
}

/**
 * @brief Creates a new enum entry
 * 
 * This function allocates and initializes a new enum entry with the given values.
 * If value_size is greater than 0, the value is copied. Otherwise, the pointer is stored.
 * If a name is provided and the NO_NAMES flag is not set, the name is duplicated.
 * 
 * @param map Pointer to the EnumMap
 * @param enum_value The enum value for the new entry
 * @param value Pointer to the value to associate
 * @param name The name to associate (optional)
 * @param out_entry Output parameter for the created entry
 * @return StemError Error code indicating success or failure
 */
static StemError stem_create_entry(EnumMap* map, int enum_value, 
                                 const void* value, const char* name, 
                                 EnumEntry* *out_entry) {
    if (!map || !out_entry) {
        return STEM_ERROR_INVALID_ARG;
    }
    
    EnumEntry* entry = malloc(sizeof(EnumEntry));
    if (!entry) {
        return STEM_ERROR_OUT_OF_MEMORY;
    }
    
    memset(entry, 0, sizeof(EnumEntry));
    entry->enum_value = enum_value;
    
    /* Copy value if needed */
    if (map->value_size > 0 && value) {
        entry->value = malloc(map->value_size);
        if (!entry->value) {
            free(entry);
            return STEM_ERROR_OUT_OF_MEMORY;
        }
        memcpy(entry->value, value, map->value_size);
    } else {
        entry->value = (void*)value;
    }
    
    /* Copy name if provided */
    if (name && !(map->flags & STEM_FLAGS_NO_NAMES)) {
        entry->name = strdup(name);
        if (!entry->name) {
            if (map->value_size > 0) {
                free(entry->value);
            }
            free(entry);
            return STEM_ERROR_OUT_OF_MEMORY;
        }
    }
    
    *out_entry = entry;
    return STEM_SUCCESS;
}

/**
 * @brief Frees an enum entry and its resources
 * 
 * This function releases all memory associated with an enum entry,
 * including the value (if copied) and the name (if present).
 * 
 * @param map Pointer to the EnumMap
 * @param entry Pointer to the entry to free
 */
static void stem_free_entry(EnumMap* map, EnumEntry* entry) {
    if (!entry) {
        return;
    }
    
    if (map->value_size > 0) {
        free(entry->value);
    }
    
    if (entry->name) {
        free(entry->name);
    }
    
    free(entry);
}

/* ==================== PUBLIC C API IMPLEMENTATION ==================== */

/**
 * @brief Creates a new enum map with precise error reporting
 * 
 * @param enum_count Number of enum entries to accommodate
 * @param value_size Size of each value in bytes (0 for pointer storage)
 * @param flags Configuration flags
 * @param error Optional error code output
 * @return EnumMap* Pointer to created enum map, or NULL on failure
 */
EnumMap* stdem_create_ex(size_t enum_count, size_t value_size, 
                        StemFlags flags, StemError* error) {
    if (enum_count == 0 || enum_count > STEM_MAX_ENTRIES) {
        if (error) {
            *error = STEM_ERROR_INVALID_ARG;
        }
        return NULL;
    }
    
    EnumMap* map = malloc(sizeof(EnumMap));
    if (!map) {
        if (error) {
            *error = STEM_ERROR_OUT_OF_MEMORY;
        }
        return NULL;
    }
    
    memset(map, 0, sizeof(EnumMap));
    map->value_size = value_size;
    map->flags = flags;
    
    /* Calculate initial bucket size based on expected entries */
    map->num_buckets = STEM_DEFAULT_BUCKETS;
    if (enum_count > map->num_buckets * STEM_LOAD_FACTOR) {
        map->num_buckets = (size_t)(enum_count / STEM_LOAD_FACTOR) + 1;
    }
    
    map->buckets = calloc(map->num_buckets, sizeof(EnumEntry*));
    if (!map->buckets) {
        free(map);
        if (error) {
            *error = STEM_ERROR_OUT_OF_MEMORY;
        }
        return NULL;
    }
    
    /* Initialize mutex to NULL (not used by default) */
    map->mutex = NULL;
    
    if (error) {
        *error = STEM_SUCCESS;
    }
    return map;
}

/**
 * @brief Destroys an enum map and releases all resources
 * 
 * @param map Pointer to the EnumMap to destroy
 */
void stdem_destroy(EnumMap* map) {
    if (!map) {
        return;
    }
    
    stem_lock_map(map);
    
    for (size_t i = 0; i < map->num_buckets; i++) {
        EnumEntry* entry = map->buckets[i];
        while (entry) {
            EnumEntry* next = entry->next;
            stem_free_entry(map, entry);
            entry = next;
        }
    }
    
    free(map->buckets);
    free(map);
}

/**
 * @brief Associates a value with an enum entry with error reporting
 * 
 * @param map Enum map to modify
 * @param enum_value Enum value to associate with
 * @param value Pointer to the value to associate
 * @param name Optional name for the enum entry
 * @return StemError Error code indicating success or failure
 */
StemError stdem_associate_ex(EnumMap* map, int enum_value, 
                           const void* value, const char* name) {
    if (!map) {
        return STEM_ERROR_INVALID_ARG;
    }
    
    if (map->flags & STEM_FLAGS_READONLY) {
        return STEM_ERROR_INVALID_ARG;
    }
    
    stem_lock_map(map);
    
    /* Check if entry already exists */
    EnumEntry* existing = stem_find_entry(map, enum_value);
    if (existing) {
        stem_unlock_map(map);
        return STEM_ERROR_ALREADY_EXISTS;
    }
    
    /* Check if we need to resize */
    if ((float)map->count / map->num_buckets > STEM_LOAD_FACTOR) {
        StemError error = stem_resize_map(map, map->num_buckets * 2);
        if (error != STEM_SUCCESS) {
            stem_unlock_map(map);
            return error;
        }
    }
    
    /* Create new entry */
    EnumEntry* new_entry;
    StemError error = stem_create_entry(map, enum_value, value, name, &new_entry);
    if (error != STEM_SUCCESS) {
        stem_unlock_map(map);
        return error;
    }
    
    /* Add to bucket */
    uint32_t hash = stem_hash_int(enum_value);
    size_t bucket_idx = hash % map->num_buckets;
    
    new_entry->next = map->buckets[bucket_idx];
    map->buckets[bucket_idx] = new_entry;
    map->count++;
    
    stem_unlock_map(map);
    return STEM_SUCCESS;
}

/**
 * @brief Retrieves a value associated with an enum entry with error reporting
 * 
 * @param map Enum map to query
 * @param enum_value Enum value to look up
 * @param error Optional error code output
 * @return const void* Pointer to the associated value, or NULL on error
 */
const void* stdem_get_value_ex(const EnumMap* map, int enum_value, StemError* error) {
    if (!map) {
        if (error) {
            *error = STEM_ERROR_INVALID_ARG;
        }
        return NULL;
    }
    
    stem_lock_map((EnumMap*)map);
    
    EnumEntry* entry = stem_find_entry(map, enum_value);
    if (!entry) {
        stem_unlock_map((EnumMap*)map);
        if (error) {
            *error = STEM_ERROR_NOT_FOUND;
        }
        return NULL;
    }
    
    stem_unlock_map((EnumMap*)map);
    
    if (error) {
        *error = STEM_SUCCESS;
    }
    return entry->value;
}

/**
 * @brief Retrieves the name associated with an enum entry with error reporting
 * 
 * @param map Enum map to query
 * @param enum_value Enum value to look up
 * @param error Optional error code output
 * @return const char* Name of the enum entry, or NULL if not found
 */
const char* stdem_get_name_ex(const EnumMap* map, int enum_value, StemError* error) {
    if (!map) {
        if (error) {
            *error = STEM_ERROR_INVALID_ARG;
        }
        return NULL;
    }
    
    if (map->flags & STEM_FLAGS_NO_NAMES) {
        if (error) {
            *error = STEM_ERROR_NOT_FOUND;
        }
        return NULL;
    }
    
    stem_lock_map((EnumMap*)map);
    
    EnumEntry* entry = stem_find_entry(map, enum_value);
    if (!entry) {
        stem_unlock_map((EnumMap*)map);
        if (error) {
            *error = STEM_ERROR_NOT_FOUND;
        }
        return NULL;
    }
    
    stem_unlock_map((EnumMap*)map);
    
    if (error) {
        *error = STEM_SUCCESS;
    }
    return entry->name;
}

/**
 * @brief Finds an enum value by its name with error reporting
 * 
 * @param map Enum map to search
 * @param name Name to search for
 * @param error Optional error code output
 * @return int Enum value associated with the name, or 0 on error
 */
int stdem_find_by_name(const EnumMap* map, const char* name, StemError* error) {
    if (!map || !name) {
        if (error) {
            *error = STEM_ERROR_INVALID_ARG;
        }
        return 0;
    }
    
    if (map->flags & STEM_FLAGS_NO_NAMES) {
        if (error) {
            *error = STEM_ERROR_NOT_FOUND;
        }
        return 0;
    }
    
    stem_lock_map((EnumMap*)map);
    
    
    for (size_t i = 0; i < map->num_buckets; i++) {
        EnumEntry* entry = map->buckets[i];
        while (entry) {
            if (entry->name && strcmp(entry->name, name) == 0) {
                stem_unlock_map((EnumMap*)map);
                if (error) {
                    *error = STEM_SUCCESS;
                }
                return entry->enum_value;
            }
            entry = entry->next;
        }
    }
    
    stem_unlock_map((EnumMap*)map);
    
    if (error) {
        *error = STEM_ERROR_NOT_FOUND;
    }
    return 0;
}

/**
 * @brief Iterates over all entries in the enum map
 * 
 * @param map Enum map to iterate over
 * @param iterator Callback function to call for each entry
 * @param user_data User context passed to the iterator
 * @return StemError Error code indicating success or failure
 */
StemError stdem_foreach(const EnumMap* map, EnumMapIterator iterator, 
                       void* user_data) {
    if (!map || !iterator) {
        return STEM_ERROR_INVALID_ARG;
    }
    
    stem_lock_map((EnumMap*)map);
    
    for (size_t i = 0; i < map->num_buckets; i++) {
        EnumEntry* entry = map->buckets[i];
        while (entry) {
            iterator(entry->enum_value, entry->name, entry->value, 
                    map->value_size, user_data);
            entry = entry->next;
        }
    }
    
    stem_unlock_map((EnumMap*)map);
    return STEM_SUCCESS;
}

/**
 * @brief Returns the number of entries in the enum map
 * 
 * @param map Enum map to query
 * @return size_t Number of entries in the map
 */
size_t stdem_count(const EnumMap* map) {
    if (!map) {
        return 0;
    }
    
    stem_lock_map((EnumMap*)map);
    size_t count = map->count;
    stem_unlock_map((EnumMap*)map);
    
    return count;
}

/**
 * @brief Returns the size of the value storage in bytes
 * 
 * @param map Enum map to query
 * @return size_t Size of each value in bytes (0 for pointer storage)
 */
size_t stdem_value_size(const EnumMap* map) {
    return map ? map->value_size : 0;
}

/**
 * @brief Clears all associations in the enum map
 * 
 * @param map Enum map to clear
 * @return StemError Error code indicating success or failure
 */
StemError stdem_clear(EnumMap* map) {
    if (!map) {
        return STEM_ERROR_INVALID_ARG;
    }
    
    if (map->flags & STEM_FLAGS_READONLY) {
        return STEM_ERROR_INVALID_ARG;
    }
    
    stem_lock_map(map);
    
    for (size_t i = 0; i < map->num_buckets; i++) {
        EnumEntry* entry = map->buckets[i];
        while (entry) {
            EnumEntry* next = entry->next;
            stem_free_entry(map, entry);
            entry = next;
        }
        map->buckets[i] = NULL;
    }
    
    map->count = 0;
    
    stem_unlock_map(map);
    return STEM_SUCCESS;
}

/**
 * @brief Creates a copy of an enum map
 * 
 * @param map Enum map to copy
 * @param error Optional error code output
 * @return EnumMap* New enum map with the same associations, or NULL on failure
 */
EnumMap* stdem_copy(const EnumMap* map, StemError* error) {
    if (!map) {
        if (error) {
            *error = STEM_ERROR_INVALID_ARG;
        }
        return NULL;
    }
    
    stem_lock_map((EnumMap*)map);
    
    EnumMap* new_map = stdem_create_ex(map->count, map->value_size, 
                                     map->flags, error);
    if (!new_map) {
        stem_unlock_map((EnumMap*)map);
        return NULL;
    }
    
    /* Copy all entries */
    StemError err = STEM_SUCCESS;
    for (size_t i = 0; i < map->num_buckets; i++) {
        EnumEntry* entry = map->buckets[i];
        while (entry) {
            err = stdem_associate_ex(new_map, entry->enum_value, 
                                   entry->value, entry->name);
            if (err != STEM_SUCCESS) {
                break;
            }
            entry = entry->next;
        }
        if (err != STEM_SUCCESS) {
            break;
        }
    }
    
    stem_unlock_map((EnumMap*)map);
    
    if (err != STEM_SUCCESS) {
        stdem_destroy(new_map);
        if (error) {
            *error = err;
        }
        return NULL;
    }
    
    if (error) {
        *error = STEM_SUCCESS;
    }
    return new_map;
}

/**
 * @brief Merges two enum maps into a new one
 * 
 * @param map1 First enum map
 * @param map2 Second enum map
 * @param overwrite If true, entries from map2 overwrite those in map1
 * @param error Optional error code output
 * @return EnumMap* New enum map containing merged associations, or NULL on failure
 */
EnumMap* stdem_merge(const EnumMap* map1, const EnumMap* map2, 
                    bool overwrite, StemError* error) {
    if (!map1 || !map2) {
        if (error) {
            *error = STEM_ERROR_INVALID_ARG;
        }
        return NULL;
    }
    
    if (map1->value_size != map2->value_size) {
        if (error) {
            *error = STEM_ERROR_INVALID_ARG;
        }
        return NULL;
    }
    
    stem_lock_map((EnumMap*)map1);
    stem_lock_map((EnumMap*)map2);
    
    StemFlags flags = map1->flags | map2->flags;
    EnumMap* new_map = stdem_create_ex(map1->count + map2->count, 
                                     map1->value_size, flags, error);
    if (!new_map) {
        stem_unlock_map((EnumMap*)map2);
        stem_unlock_map((EnumMap*)map1);
        return NULL;
    }
    
    /* Copy from first map */
    StemError err = STEM_SUCCESS;
    for (size_t i = 0; i < map1->num_buckets; i++) {
        EnumEntry* entry = map1->buckets[i];
        while (entry) {
            err = stdem_associate_ex(new_map, entry->enum_value, 
                                   entry->value, entry->name);
            if (err != STEM_SUCCESS) {
                break;
            }
            entry = entry->next;
        }
        if (err != STEM_SUCCESS) {
            break;
        }
    }
    
    if (err != STEM_SUCCESS) {
        stem_unlock_map((EnumMap*)map2);
        stem_unlock_map((EnumMap*)map1);
        stdem_destroy(new_map);
        if (error) {
            *error = err;
        }
        return NULL;
    }
    
    /* Copy from second map */
    for (size_t i = 0; i < map2->num_buckets; i++) {
        EnumEntry* entry = map2->buckets[i];
        while (entry) {
            /* Check if entry already exists in new map */
            EnumEntry* existing = stem_find_entry(new_map, entry->enum_value);
            if (existing) {
                if (overwrite) {
                    /* Update existing entry */
                    if (new_map->value_size > 0) {
                        memcpy(existing->value, entry->value, new_map->value_size);
                    } else {
                        existing->value = entry->value;
                    }
                    
                    /* Update name if needed */
                    if (existing->name) {
                        free(existing->name);
                    }
                    if (entry->name && !(new_map->flags & STEM_FLAGS_NO_NAMES)) {
                        existing->name = strdup(entry->name);
                        if (!existing->name) {
                            err = STEM_ERROR_OUT_OF_MEMORY;
                            break;
                        }
                    } else {
                        existing->name = NULL;
                    }
                }
                /* If not overwriting, just skip this entry */
            } else {
                err = stdem_associate_ex(new_map, entry->enum_value, 
                                       entry->value, entry->name);
                if (err != STEM_SUCCESS) {
                    break;
                }
            }
            entry = entry->next;
        }
        if (err != STEM_SUCCESS) {
            break;
        }
    }
    
    stem_unlock_map((EnumMap*)map2);
    stem_unlock_map((EnumMap*)map1);
    
    if (err != STEM_SUCCESS) {
        stdem_destroy(new_map);
        if (error) {
            *error = err;
        }
        return NULL;
    }
    
    if (error) {
        *error = STEM_SUCCESS;
    }
    return new_map;
}

/**
 * @brief Returns the last error that occurred in the current thread
 * 
 * @return StemError Last error code
 */
StemError stdem_get_last_error(void) {
    /* Thread-local storage would be needed for proper implementation
     * For maximum portability, we use a static variable
     */
    static StemError last_error = STEM_SUCCESS;
    return last_error;
}

/**
 * @brief Returns a human-readable description of an error code
 * 
 * @param error Error code to describe
 * @return const char* String describing the error
 */
const char* stdem_error_string(StemError error) {
    switch (error) {
        case STEM_SUCCESS: return "Success";
        case STEM_ERROR_INVALID_ARG: return "Invalid argument";
        case STEM_ERROR_OUT_OF_MEMORY: return "Out of memory";
        case STEM_ERROR_INDEX_OUT_OF_BOUNDS: return "Index out of bounds";
        case STEM_ERROR_NOT_FOUND: return "Not found";
        case STEM_ERROR_ALREADY_EXISTS: return "Already exists";
        case STEM_ERROR_UNINITIALIZED: return "Uninitialized";
        default: return "Unknown error";
    }
}

/* ==================== ADDITIONAL UTILITY FUNCTIONS ==================== */

/**
 * @brief Serializes an enum map to a binary stream
 * 
 * @param map The enum map to serialize
 * @param stream The output stream (FILE*)
 * @return StemError Error code indicating success or failure
 */
StemError stdem_serialize(const EnumMap* map, FILE* stream) {
    if (!map || !stream) {
        return STEM_ERROR_INVALID_ARG;
    }
    
    stem_lock_map((EnumMap*)map);
    
    /* Write header: magic number, version, count, value_size, flags */
    const uint32_t magic = 0x454E554D; /* 'ENUM' */
    const uint16_t version = 1;
    
    if (fwrite(&magic, sizeof(magic), 1, stream) != 1 ||
        fwrite(&version, sizeof(version), 1, stream) != 1 ||
        fwrite(&map->count, sizeof(map->count), 1, stream) != 1 ||
        fwrite(&map->value_size, sizeof(map->value_size), 1, stream) != 1 ||
        fwrite(&map->flags, sizeof(map->flags), 1, stream) != 1) {
        stem_unlock_map((EnumMap*)map);
        return STEM_ERROR_INVALID_ARG;
    }
    
    /* Write entries */
    for (size_t i = 0; i < map->num_buckets; i++) {
        EnumEntry* entry = map->buckets[i];
        while (entry) {
            if (fwrite(&entry->enum_value, sizeof(entry->enum_value), 1, stream) != 1) {
                stem_unlock_map((EnumMap*)map);
                return STEM_ERROR_INVALID_ARG;
            }
            
            /* Write name length and name */
            uint16_t name_len = entry->name ? (uint16_t)strlen(entry->name) : 0;
            if (fwrite(&name_len, sizeof(name_len), 1, stream) != 1) {
                stem_unlock_map((EnumMap*)map);
                return STEM_ERROR_INVALID_ARG;
            }
            
            if (name_len > 0) {
                if (fwrite(entry->name, 1, name_len, stream) != name_len) {
                    stem_unlock_map((EnumMap*)map);
                    return STEM_ERROR_INVALID_ARG;
                }
            }
            
            /* Write value */
            if (map->value_size > 0) {
                if (fwrite(entry->value, 1, map->value_size, stream) != map->value_size) {
                    stem_unlock_map((EnumMap*)map);
                    return STEM_ERROR_INVALID_ARG;
                }
            } else {
                if (fwrite(&entry->value, sizeof(entry->value), 1, stream) != 1) {
                    stem_unlock_map((EnumMap*)map);
                    return STEM_ERROR_INVALID_ARG;
                }
            }
            
            entry = entry->next;
        }
    }
    
    stem_unlock_map((EnumMap*)map);
    return STEM_SUCCESS;
}

/**
 * @brief Deserializes an enum map from a binary stream
 * 
 * @param stream The input stream (FILE*)
 * @param error Optional error output
 * @return EnumMap* New enum map, or NULL on error
 */
EnumMap* stdem_deserialize(FILE* stream, StemError* error) {
    if (!stream) {
        if (error) {
            *error = STEM_ERROR_INVALID_ARG;
        }
        return NULL;
    }
    
    /* Read and validate header */
    uint32_t magic;
    uint16_t version;
    size_t count;
    size_t value_size;
    StemFlags flags;
    
    if (fread(&magic, sizeof(magic), 1, stream) != 1 || magic != 0x454E554D) {
        if (error) {
            *error = STEM_ERROR_INVALID_ARG;
        }
        return NULL;
    }
    
    if (fread(&version, sizeof(version), 1, stream) != 1 || version != 1) {
        if (error) {
            *error = STEM_ERROR_INVALID_ARG;
        }
        return NULL;
    }
    
    if (fread(&count, sizeof(count), 1, stream) != 1 ||
        fread(&value_size, sizeof(value_size), 1, stream) != 1 ||
        fread(&flags, sizeof(flags), 1, stream) != 1) {
        if (error) {
            *error = STEM_ERROR_INVALID_ARG;
        }
        return NULL;
    }
    
    /* Create new map */
    EnumMap* map = stdem_create_ex(count, value_size, flags, error);
    if (!map) {
        return NULL;
    }
    
    /* Read entries */
    for (size_t i = 0; i < count; i++) {
        int enum_value;
        if (fread(&enum_value, sizeof(enum_value), 1, stream) != 1) {
            stdem_destroy(map);
            if (error) {
                *error = STEM_ERROR_INVALID_ARG;
            }
            return NULL;
        }
        
        uint16_t name_len;
        if (fread(&name_len, sizeof(name_len), 1, stream) != 1) {
            stdem_destroy(map);
            if (error) {
                *error = STEM_ERROR_INVALID_ARG;
            }
            return NULL;
        }
        
        char* name = NULL;
        if (name_len > 0) {
            name = malloc(name_len + 1);
            if (!name) {
                stdem_destroy(map);
                if (error) {
                    *error = STEM_ERROR_OUT_OF_MEMORY;
                }
                return NULL;
            }
            if (fread(name, 1, name_len, stream) != name_len) {
                free(name);
                stdem_destroy(map);
                if (error) {
                    *error = STEM_ERROR_INVALID_ARG;
                }
                return NULL;
            }
            name[name_len] = '\0';
        }
        
        void* value;
        if (value_size > 0) {
            value = malloc(value_size);
            if (!value) {
                if (name) {
                    free(name);
                }
                stdem_destroy(map);
                if (error) {
                    *error = STEM_ERROR_OUT_OF_MEMORY;
                }
                return NULL;
            }
            if (fread(value, 1, value_size, stream) != value_size) {
                free(value);
                if (name) {
                    free(name);
                }
                stdem_destroy(map);
                if (error) {
                    *error = STEM_ERROR_INVALID_ARG;
                }
                return NULL;
            }
        } else {
            if (fread(&value, sizeof(value), 1, stream) != 1) {
                if (name) {
                    free(name);
                }
                stdem_destroy(map);
                if (error) {
                    *error = STEM_ERROR_INVALID_ARG;
                }
                return NULL;
            }
        }
        
        StemError err = stdem_associate_ex(map, enum_value, value, name);
        if (name) {
            free(name);
        }
        if (value_size > 0) {
            free(value);
        }
        
        if (err != STEM_SUCCESS) {
            stdem_destroy(map);
            if (error) {
                *error = err;
            }
            return NULL;
        }
    }
    
    if (error) {
        *error = STEM_SUCCESS;
    }
    return map;
}