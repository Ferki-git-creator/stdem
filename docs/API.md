Standard Enum Mapping Library - API Documentation

Introduction

The Standard Enum Mapping Library (STEM) provides an efficient way to map enum values to arbitrary data with constant-time access, minimal memory footprint, and zero dependencies. The library is designed for use in embedded systems, game development, and high-performance applications.

License

The library is distributed under the LGPL-3.0-or-later license, making it compatible with GNU projects and allowing use in commercial products.

Data Structures

EnumMap

An opaque structure that encapsulates all internal data needed for efficient enum value mapping. Clients should treat this structure as opaque and access it only through the provided API functions.

```c
typedef struct EnumMap EnumMap;
```

StemError

Enumeration of error codes returned by various library functions.

```c
typedef enum {
    STEM_SUCCESS = 0,          // Operation completed successfully
    STEM_ERROR_INVALID_ARG,    // Invalid argument provided
    STEM_ERROR_OUT_OF_MEMORY,  // Memory allocation failed
    STEM_ERROR_INDEX_OUT_OF_BOUNDS, // Index out of valid range
    STEM_ERROR_NOT_FOUND,      // Requested item not found
    STEM_ERROR_ALREADY_EXISTS, // Item already exists
    STEM_ERROR_UNINITIALIZED,  // Map not properly initialized
} StemError;
```

StemFlags

Configuration flags for enum map creation.

```c
typedef enum {
    STEM_FLAGS_NONE = 0,           // Default behavior
    STEM_FLAGS_NO_NAMES = 1 << 0,  // Don't store enum names to save memory
    STEM_FLAGS_READONLY = 1 << 1,  // Create read-only map (optimized for access)
    STEM_FLAGS_COPY_VALUES = 1 << 2, // Copy values instead of storing pointers
} StemFlags;
```

EnumMapIterator

Function type for iterating over enum map entries.

```c
typedef void (*EnumMapIterator)(int enum_value, const char* enum_name, 
                               const void* value, size_t value_size, 
                               void* user_data);
```

Functions

Creation and Destruction

stdem_create_ex

```c
EnumMap* stdem_create_ex(size_t enum_count, size_t value_size, 
                        StemFlags flags, StemError* error);
```

Creates a new enum map with precise error reporting.

Parameters:

· enum_count: Number of enum entries to accommodate
· value_size: Size of each value in bytes (0 for pointer storage)
· flags: Configuration flags
· error: Optional error code output

Returns:

· Pointer to created enum map, or NULL on failure

Notes:

· If value_size is 0, values are stored as pointers
· In case of error, the error code is written to error (if not NULL)

stdem_create

```c
static inline EnumMap* stdem_create(size_t enum_count, size_t value_size);
```

Simplified enum map creation with default flags.

stdem_destroy

```c
void stdem_destroy(EnumMap* map);
```

Destroys an enum map and releases all resources.

Parameters:

· map: Pointer to the EnumMap to destroy

Association and Access

stdem_associate_ex

```c
StemError stdem_associate_ex(EnumMap* map, int enum_value, 
                           const void* value, const char* name);
```

Associates a value with an enum entry with error reporting.

Parameters:

· map: Enum map to modify
· enum_value: Enum value to associate with
· value: Pointer to the value to associate
· name: Optional name for the enum entry

Returns:

· Error code indicating success or failure

Notes:

· If the map was created with STEM_FLAGS_COPY_VALUES, the value is copied internally
· Otherwise, only the pointer is stored

stdem_associate

```c
static inline bool stdem_associate(EnumMap* map, int enum_value, 
                                 const void* value, const char* name);
```

Simplified value association that returns boolean result.

stdem_get_value_ex

```c
const void* stdem_get_value_ex(const EnumMap* map, int enum_value, StemError* error);
```

Retrieves a value associated with an enum entry with error reporting.

Parameters:

· map: Enum map to query
· enum_value: Enum value to look up
· error: Optional error code output

Returns:

· Pointer to the associated value, or NULL on error

Notes:

· The returned pointer remains valid until the map is destroyed

stdem_get_value

```c
static inline const void* stdem_get_value(const EnumMap* map, int enum_value);
```

Simplified value retrieval without error reporting.

stdem_get_value_as

```c
#define stdem_get_value_as(map, enum_value, type) \
    ((const type*)stdem_get_value_ex(map, enum_value, NULL))
```

Macro for type-safe value retrieval with casting.

stdem_get_name_ex

```c
const char* stdem_get_name_ex(const EnumMap* map, int enum_value, StemError* error);
```

Retrieves the name associated with an enum entry with error reporting.

Parameters:

· map: Enum map to query
· enum_value: Enum value to look up
· error: Optional error code output

Returns:

· Name of the enum entry, or NULL if not found or not stored

stdem_get_name

```c
static inline const char* stdem_get_name(const EnumMap* map, int enum_value);
```

Simplified name retrieval without error reporting.

Search and Iteration

stdem_find_by_name

```c
int stdem_find_by_name(const EnumMap* map, const char* name, StemError* error);
```

Finds an enum value by its name.

Parameters:

· map: Enum map to search
· name: Name to search for
· error: Optional error code output

Returns:

· Enum value associated with the name, or 0 on error

stdem_foreach

```c
StemError stdem_foreach(const EnumMap* map, EnumMapIterator iterator, 
                       void* user_data);
```

Iterates over all entries in the enum map.

Parameters:

· map: Enum map to iterate over
· iterator: Callback function to call for each entry
· user_data: User context passed to the iterator

Returns:

· Error code indicating success or failure

Utilities

stdem_count

```c
size_t stdem_count(const EnumMap* map);
```

Returns the number of entries in the enum map.

Parameters:

· map: Enum map to query

Returns:

· Number of entries in the map

stdem_value_size

```c
size_t stdem_value_size(const EnumMap* map);
```

Returns the size of the value storage in bytes.

Parameters:

· map: Enum map to query

Returns:

· Size of each value in bytes (0 for pointer storage)

stdem_clear

```c
StemError stdem_clear(EnumMap* map);
```

Clears all associations in the enum map.

Parameters:

· map: Enum map to clear

Returns:

· Error code indicating success or failure

stdem_copy

```c
EnumMap* stdem_copy(const EnumMap* map, StemError* error);
```

Creates a copy of an enum map.

Parameters:

· map: Enum map to copy
· error: Optional error code output

Returns:

· New enum map with the same associations, or NULL on failure

stdem_merge

```c
EnumMap* stdem_merge(const EnumMap* map1, const EnumMap* map2, 
                    bool overwrite, StemError* error);
```

Merges two enum maps into a new one.

Parameters:

· map1: First enum map
· map2: Second enum map
· overwrite: If true, entries from map2 overwrite those in map1
· error: Optional error code output

Returns:

· New enum map containing merged associations, or NULL on failure

Error Handling

stdem_get_last_error

```c
StemError stdem_get_last_error(void);
```

Returns the last error that occurred in the current thread.

Returns:

· Last error code

stdem_error_string

```c
const char* stdem_error_string(StemError error);
```

Returns a human-readable description of an error code.

Parameters:

· error: Error code to describe

Returns:

· String describing the error

Additional Utilities

stdem_exists

```c
static inline bool stdem_exists(const EnumMap* map, int enum_value);
```

Checks if an enum value exists in the map.

stdem_get_value_or

```c
static inline const void* stdem_get_value_or(const EnumMap* map, int enum_value, 
                                           const void* default_value);
```

Retrieves a value with a default fallback.

stdem_get_value_or_default

```c
#define stdem_get_value_or_default(map, enum_value, type, default_val) \
    (stdem_exists(map, enum_value) ? *stdem_get_value_as(map, enum_value, type) : default_val)
```

Macro for type-safe value retrieval with default fallback.

Serialization

stdem_serialize

```c
StemError stdem_serialize(const EnumMap* map, FILE* stream);
```

Serializes an enum map to a binary stream.

Parameters:

· map: The enum map to serialize
· stream: The output stream (FILE*)

Returns:

· Error code indicating success or failure

stdem_deserialize

```c
EnumMap* stdem_deserialize(FILE* stream, StemError* error);
```

Deserializes an enum map from a binary stream.

Parameters:

· stream: The input stream (FILE*)
· error: Optional error output

Returns:

· New enum map, or NULL on error

Usage Examples

Detailed usage examples can be found in USAGE.md.

Notes

1. The library is completely thread-safe when used with external synchronization mechanisms
2. All functions that take an EnumMap pointer check it for NULL
3. The library does not use dynamic memory during operation unless creation/destruction functions are called
4. For maximum performance, it is recommended to use STEM_FLAGS_COPY_VALUES mode for small data types
5. STEM_FLAGS_NO_NAMES mode significantly reduces memory consumption if names are not needed

Standards Compliance

The library complies with the C99 standard and can be used with C++ without additional modifications.