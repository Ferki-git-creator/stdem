Usage Guide - Standard Enum Mapping Library

Table of Contents

1. Basic Usage
2. Error Handling
3. Advanced Features
4. Serialization
5. Thread Safety
6. Performance Considerations
7. C++ Integration
8. Embedded Systems

Basic Usage

Including the Library

```c
#include <stdem.h>
```

Creating an Enum Map

```c
// Create a map for 5 enum entries with integer values
StemError error;
EnumMap* map = stdem_create_ex(5, sizeof(int), STEM_FLAGS_NONE, &error);

if (!map) {
    printf("Failed to create map: %s\n", stdem_error_string(error));
    return 1;
}
```

Associating Values

```c
typedef enum {
    STATE_IDLE,
    STATE_ACTIVE, 
    STATE_ERROR,
    STATE_SHUTDOWN,
    STATE_INIT
} SystemState;

// Associate values with enum entries
int idle_value = 100;
stdem_associate_ex(map, STATE_IDLE, &idle_value, "STATE_IDLE");

int active_value = 200;
stdem_associate_ex(map, STATE_ACTIVE, &active_value, "STATE_ACTIVE");

// ... associate other states
```

Retrieving Values

```c
// Get value by enum
const int* value = stdem_get_value_ex(map, STATE_ACTIVE, &error);
if (value) {
    printf("Active state value: %d\n", *value);
}

// Get name by enum
const char* name = stdem_get_name_ex(map, STATE_IDLE, &error);
if (name) {
    printf("State name: %s\n", name);
}

// Find enum by name
int found_state = stdem_find_by_name(map, "STATE_ERROR", &error);
if (error == STEM_SUCCESS) {
    printf("Found state: %d\n", found_state);
}
```

Iterating Over Entries

```c
void print_entry(int enum_value, const char* name, const void* value, 
                 size_t value_size, void* user_data) {
    printf("Enum: %d, Name: %s, Value: %d\n", 
           enum_value, name, *(const int*)value);
}

stdem_foreach(map, print_entry, NULL);
```

Cleaning Up

```c
stdem_destroy(map);
```

Error Handling

The library provides comprehensive error handling through the StemError enum:

```c
EnumMap* map = stdem_create_ex(10, sizeof(int), STEM_FLAGS_NONE, &error);
if (!map) {
    switch (error) {
        case STEM_ERROR_OUT_OF_MEMORY:
            printf("Out of memory!\n");
            break;
        case STEM_ERROR_INVALID_ARG:
            printf("Invalid arguments!\n");
            break;
        default:
            printf("Unknown error: %s\n", stdem_error_string(error));
    }
    return 1;
}
```

Simplified Error Checking

```c
// For functions that return pointers
const int* value = stdem_get_value_ex(map, 999, &error);
if (!value) {
    printf("Error: %s\n", stdem_error_string(error));
}

// For functions that return StemError directly
StemError result = stdem_clear(map);
if (result != STEM_SUCCESS) {
    printf("Error: %s\n", stdem_error_string(result));
}
```

Advanced Features

Using Flags

```c
// Create a map without storing names (saves memory)
EnumMap* map = stdem_create_ex(10, sizeof(int), STEM_FLAGS_NO_NAMES, NULL);

// Create a read-only map (optimized for access)
EnumMap* ro_map = stdem_create_ex(5, sizeof(float), STEM_FLAGS_READONLY, NULL);

// Create a map that copies values instead of storing pointers
EnumMap* copy_map = stdem_create_ex(8, sizeof(double), STEM_FLAGS_COPY_VALUES, NULL);
```

Copying and Merging Maps

```c
// Create a copy of a map
EnumMap* copy = stdem_copy(original_map, &error);

// Merge two maps
EnumMap* merged = stdem_merge(map1, map2, false, &error); // Don't overwrite
EnumMap* merged_ov = stdem_merge(map1, map2, true, &error); // Overwrite
```

Type-Safe Access Macros

```c
// Safe value retrieval with type casting
const int* value = stdem_get_value_as(map, STATE_ACTIVE, int);

// Value retrieval with default fallback
int value_or_default = stdem_get_value_or_default(map, STATE_UNKNOWN, int, -1);
```

Serialization

Saving a Map to File

```c
FILE* file = fopen("map.bin", "wb");
if (file) {
    StemError error = stdem_serialize(map, file);
    fclose(file);
    
    if (error != STEM_SUCCESS) {
        printf("Serialization failed: %s\n", stdem_error_string(error));
    }
}
```

Loading a Map from File

```c
FILE* file = fopen("map.bin", "rb");
if (file) {
    StemError error;
    EnumMap* loaded_map = stdem_deserialize(file, &error);
    fclose(file);
    
    if (!loaded_map) {
        printf("Deserialization failed: %s\n", stdem_error_string(error));
    } else {
        // Use the loaded map
        stdem_destroy(loaded_map);
    }
}
```

Thread Safety

The library provides thread-safe operations when used with external synchronization:

```c
#include <pthread.h>

pthread_mutex_t map_mutex = PTHREAD_MUTEX_INITIALIZER;

// Thread-safe access
pthread_mutex_lock(&map_mutex);
const int* value = stdem_get_value_ex(map, STATE_ACTIVE, NULL);
// Use value...
pthread_mutex_unlock(&map_mutex);
```

For platforms without pthreads, use appropriate synchronization primitives.

Performance Considerations

1. Value Size: For small data types (≤ pointer size), use STEM_FLAGS_COPY_VALUES
2. Memory vs Speed: STEM_FLAGS_NO_NAMES reduces memory usage but eliminates name-based lookup
3. Read-Only Maps: STEM_FLAGS_READONLY enables optimizations for maps that won't change
4. Pre-allocation: Create maps with the expected number of entries to minimize resizing

```c
// Optimal for small values
EnumMap* optimized = stdem_create_ex(
    expected_count, 
    sizeof(int), 
    STEM_FLAGS_COPY_VALUES | STEM_FLAGS_NO_NAMES,
    NULL
);
```

C++ Integration

The library provides seamless C++ integration:

```cpp
#include "stdem.h"

// Using the C++ wrapper
stem::EnumMap map(5, sizeof(int));

// Associate values
int value = 42;
map.associate(STATE_IDLE, value, "STATE_IDLE");

// Retrieve values
try {
    int retrieved = map.get<int>(STATE_IDLE);
    std::cout << "Value: " << retrieved << std::endl;
} catch (const std::runtime_error& e) {
    std::cerr << "Error: " << e.what() << std::endl;
}

// Iteration with lambdas
map.for_each([](int enum_val, const char* name, const void* val, size_t size) {
    std::cout << enum_val << ": " << name << " = " << *(const int*)val << std::endl;
});
```

STL Integration

```cpp
// Get all keys
std::vector<int> keys = map.keys();

// Get all names  
std::vector<std::string> names = map.names();

// Check if a key exists
if (map.contains(STATE_ACTIVE)) {
    // Key exists
}

// Array-style access
int value = map[STATE_IDLE];
```

Embedded Systems

The library is ideal for embedded systems with limited resources:

```c
// For memory-constrained systems
EnumMap* embedded_map = stdem_create_ex(
    8,                          // 8 entries
    0,                          // Store pointers, not values
    STEM_FLAGS_NO_NAMES,        // Don't store names
    NULL
);

// Associate with static data
static const int config_values[] = {100, 200, 300, 400};
stdem_associate_ex(embedded_map, CONFIG_1, &config_values[0], NULL);
stdem_associate_ex(embedded_map, CONFIG_2, &config_values[1], NULL);

// In interrupt handlers, avoid allocation/deallocation
// Pre-allocate all needed maps during initialization
```

Bare Metal Systems

For bare metal systems without malloc/free:

1. Custom allocator: Implement your own memory management
2. Static allocation: Pre-allocate maps during initialization
3. Pool allocation: Use a fixed-size memory pool

```c
// Example for systems without dynamic memory
static EnumMap static_map;
static EnumEntry static_entries[10];
static EnumEntry* static_buckets[8];

void init_static_map() {
    static_map.count = 0;
    static_map.value_size = sizeof(int);
    static_map.flags = STEM_FLAGS_COPY_VALUES;
    static_map.buckets = static_buckets;
    static_map.num_buckets = 8;
    // Initialize buckets to NULL
    for (int i = 0; i < 8; i++) {
        static_buckets[i] = NULL;
    }
}
```

Best Practices

1. Always check return values for errors
2. Use appropriate flags for your use case
3. Pre-allocate maps with the expected number of entries
4. Destroy maps when no longer needed to prevent memory leaks
5. Use type-safe macros when possible to avoid casting errors
6. Consider memory constraints when choosing between pointer storage and value copying

Examples Repository

For more complete examples, check the examples/ directory in the source repository, which includes:

· Basic usage examples
· Advanced configuration examples
· Embedded system examples
· Multithreading examples
· Serialization examples

Support

If you encounter issues or have questions:

1. Check the API Documentation
2. Review the Installation Guide
3. Search the GitHub Issues
4. Create a new issue with a detailed description of your problem

License Reminder

This library is distributed under the LGPL-3.0-or-later license. Please ensure your usage complies with the license terms.