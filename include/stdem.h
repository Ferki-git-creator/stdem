/**
 * @file stdem.h
 * @brief Standard Enum Mapping Library - Zero-dependency enum-to-value mapping for embedded systems and high-performance applications
 * @author Ferki
 * @license LGPL-3.0-or-later
 * 
 * @copyright Copyright (c) 2025 Ferki. Permission is granted to copy, distribute
 * and/or modify this document under the terms of the GNU Lesser General Public
 * License, Version 3 or any later version published by the Free Software Foundation.
 */

#ifndef STDEM_H
#define STDEM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @defgroup stdem Standard Enum Mapping Library
 * @brief Lightweight, portable enum-to-value mapping library for C/C++
 * 
 * This library provides a efficient way to map enum values to arbitrary data
 * with constant-time access, minimal memory footprint, and zero dependencies.
 * Suitable for embedded systems, game development, and high-performance applications.
 * 
 * @{
 */

/**
 * @brief Opaque structure representing an enum map
 */
typedef struct EnumMap EnumMap;

/**
 * @brief Callback function type for iterating over enum map entries
 */
typedef void (*EnumMapIterator)(int enum_value, const char* enum_name, 
                               const void* value, size_t value_size, 
                               void* user_data);

/**
 * @brief Error codes returned by various stdem functions
 */
typedef enum {
    STEM_SUCCESS = 0,
    STEM_ERROR_INVALID_ARG,
    STEM_ERROR_OUT_OF_MEMORY,
    STEM_ERROR_INDEX_OUT_OF_BOUNDS,
    STEM_ERROR_NOT_FOUND,
    STEM_ERROR_ALREADY_EXISTS,
    STEM_ERROR_UNINITIALIZED,
} StemError;

/**
 * @brief Configuration flags for enum map creation
 */
typedef enum {
    STEM_FLAGS_NONE = 0,
    STEM_FLAGS_NO_NAMES = 1 << 0,
    STEM_FLAGS_READONLY = 1 << 1,
    STEM_FLAGS_COPY_VALUES = 1 << 2,
} StemFlags;

/* ==================== CORE C API ==================== */

/**
 * @brief Creates a new enum map with precise error reporting
 */
EnumMap* stdem_create_ex(size_t enum_count, size_t value_size, 
                        StemFlags flags, StemError* error);

/**
 * @brief Simplified enum map creation
 */
static inline EnumMap* stdem_create(size_t enum_count, size_t value_size) {
    return stdem_create_ex(enum_count, value_size, STEM_FLAGS_NONE, NULL);
}

/**
 * @brief Destroys an enum map and releases all resources
 */
void stdem_destroy(EnumMap* map);

/**
 * @brief Associates a value with an enum entry with error reporting
 */
StemError stdem_associate_ex(EnumMap* map, int enum_value, const void* value, 
                           const char* name);

/**
 * @brief Simplified value association
 */
static inline bool stdem_associate(EnumMap* map, int enum_value, 
                                  const void* value, const char* name) {
    return stdem_associate_ex(map, enum_value, value, name) == STEM_SUCCESS;
}

/**
 * @brief Retrieves a value with error reporting
 */
const void* stdem_get_value_ex(const EnumMap* map, int enum_value, StemError* error);

/**
 * @brief Simplified value retrieval
 */
static inline const void* stdem_get_value(const EnumMap* map, int enum_value) {
    return stdem_get_value_ex(map, enum_value, NULL);
}

/**
 * @brief Retrieves value with type safety
 */
#define stdem_get_value_as(map, enum_value, type) \
    ((const type*)stdem_get_value_ex(map, enum_value, NULL))

/**
 * @brief Retrieves the name with error reporting
 */
const char* stdem_get_name_ex(const EnumMap* map, int enum_value, StemError* error);

/**
 * @brief Simplified name retrieval
 */
static inline const char* stdem_get_name(const EnumMap* map, int enum_value) {
    return stdem_get_name_ex(map, enum_value, NULL);
}

/**
 * @brief Finds an enum value by its name
 */
int stdem_find_by_name(const EnumMap* map, const char* name, StemError* error);

/**
 * @brief Iterates over all entries in the enum map
 */
StemError stdem_foreach(const EnumMap* map, EnumMapIterator iterator, 
                       void* user_data);

/**
 * @brief Returns the number of entries in the enum map
 */
size_t stdem_count(const EnumMap* map);

/**
 * @brief Returns the size of the value storage in bytes
 */
size_t stdem_value_size(const EnumMap* map);

/**
 * @brief Clears all associations in the enum map
 */
StemError stdem_clear(EnumMap* map);

/**
 * @brief Creates a copy of an enum map
 */
EnumMap* stdem_copy(const EnumMap* map, StemError* error);

/**
 * @brief Merges two enum maps into a new one
 */
EnumMap* stdem_merge(const EnumMap* map1, const EnumMap* map2, 
                    bool overwrite, StemError* error);

/**
 * @brief Returns the last error that occurred
 */
StemError stdem_get_last_error(void);

/**
 * @brief Returns a human-readable error description
 */
const char* stdem_error_string(StemError error);

/**
 * @brief Checks if enum value exists in the map
 */
static inline bool stdem_exists(const EnumMap* map, int enum_value) {
    return stdem_get_value_ex(map, enum_value, NULL) != NULL;
}

/**
 * @brief Quick value retrieval with default fallback
 */
static inline const void* stdem_get_value_or(const EnumMap* map, int enum_value, 
                                           const void* default_value) {
    const void* value = stdem_get_value_ex(map, enum_value, NULL);
    return value ? value : default_value;
}

/**
 * @brief Type-safe value retrieval with default
 */
#define stdem_get_value_or_default(map, enum_value, type, default_val) \
    (stdem_exists(map, enum_value) ? *stdem_get_value_as(map, enum_value, type) : default_val)

/** @} */ // end of group stdem

#ifdef __cplusplus
}

#include <initializer_list>
#include <utility>
#include <stdexcept>
#include <functional>
#include <vector>
#include <string>
#include <type_traits>

/**
 * @brief C++ wrapper for the Standard Enum Mapping Library
 */
namespace stem {

/**
 * @brief C++ RAII wrapper for EnumMap
 */
class EnumMap {
private:
    ::EnumMap* map_;
    
public:
    using Iterator = std::function<void(int, const char*, const void*, size_t)>;
    
    /**
     * @brief Constructs a new EnumMap
     */
    EnumMap(size_t enum_count, size_t value_size = 0, 
            StemFlags flags = STEM_FLAGS_NONE) {
        StemError error;
        map_ = ::stdem_create_ex(enum_count, value_size, flags, &error);
        if (!map_) {
            throw std::runtime_error(::stdem_error_string(error));
        }
    }
    
    /**
     * @brief Construct from initializer list
     */
    template<typename T>
    EnumMap(std::initializer_list<std::pair<int, T>> init_list,
            size_t value_size = sizeof(T),
            StemFlags flags = STEM_FLAGS_COPY_VALUES) {
        StemError error;
        map_ = ::stdem_create_ex(init_list.size(), value_size, flags, &error);
        if (!map_) {
            throw std::runtime_error(::stdem_error_string(error));
        }
        
        for (const auto& pair : init_list) {
            associate(pair.first, &pair.second);
        }
    }
    
    ~EnumMap() {
        if (map_) {
            ::stdem_destroy(map_);
        }
    }
    
    EnumMap(const EnumMap&) = delete;
    EnumMap& operator=(const EnumMap&) = delete;
    
    EnumMap(EnumMap&& other) noexcept : map_(other.map_) {
        other.map_ = nullptr;
    }
    
    EnumMap& operator=(EnumMap&& other) noexcept {
        if (this != &other) {
            if (map_) {
                ::stdem_destroy(map_);
            }
            map_ = other.map_;
            other.map_ = nullptr;
        }
        return *this;
    }
    
    /**
     * @brief Associates a value with an enum entry
     */
    void associate(int enum_value, const void* value, const char* name = nullptr) {
        StemError error = ::stdem_associate_ex(map_, enum_value, value, name);
        if (error != STEM_SUCCESS) {
            throw std::runtime_error(::stdem_error_string(error));
        }
    }
    
    /**
     * @brief Type-safe value association
     */
    template<typename T>
    void associate(int enum_value, const T& value, const char* name = nullptr) {
        associate(enum_value, &value, name);
    }
    
    /**
     * @brief Retrieves a value with type safety
     */
    template<typename T>
    const T& get(int enum_value) const {
        const T* value = static_cast<const T*>(::stdem_get_value_ex(map_, enum_value, nullptr));
        if (!value) {
            throw std::runtime_error("Enum value not found");
        }
        return *value;
    }
    
    /**
     * @brief Safe value retrieval with default
     */
    template<typename T>
    T get_or(int enum_value, const T& default_value) const {
        const T* value = static_cast<const T*>(::stdem_get_value_ex(map_, enum_value, nullptr));
        return value ? *value : default_value;
    }
    
    /**
     * @brief Checks if enum value exists
     */
    bool contains(int enum_value) const {
        return ::stdem_get_value_ex(map_, enum_value, nullptr) != nullptr;
    }
    
    /**
     * @brief Returns the number of entries
     */
    size_t size() const {
        return ::stdem_count(map_);
    }
    
    /**
     * @brief Returns value size in bytes
     */
    size_t value_size() const {
        return ::stdem_value_size(map_);
    }
    
    /**
     * @brief Clears all associations
     */
    void clear() {
        StemError error = ::stdem_clear(map_);
        if (error != STEM_SUCCESS) {
            throw std::runtime_error(::stdem_error_string(error));
        }
    }
    
    /**
     * @brief Iterates over all entries
     */
    void for_each(Iterator iterator) const {
        auto c_iterator = [](int ev, const char* en, const void* v, size_t vs, void* ud) {
            (*static_cast<Iterator*>(ud))(ev, en, v, vs);
        };
        
        StemError error = ::stdem_foreach(map_, c_iterator, &iterator);
        if (error != STEM_SUCCESS) {
            throw std::runtime_error(::stdem_error_string(error));
        }
    }
    
    /**
     * @brief Gets all enum values
     */
    std::vector<int> keys() const {
        std::vector<int> result;
        for_each([&](int ev, const char*, const void*, size_t) {
            result.push_back(ev);
        });
        return result;
    }
    
    /**
     * @brief Gets all enum names
     */
    std::vector<std::string> names() const {
        std::vector<std::string> result;
        for_each([&](int, const char* en, const void*, size_t) {
            if (en) result.push_back(en);
        });
        return result;
    }
    
    /**
     * @brief Finds enum value by name
     */
    int find(const std::string& name) const {
        StemError error;
        int result = ::stdem_find_by_name(map_, name.c_str(), &error);
        if (error != STEM_SUCCESS) {
            throw std::runtime_error(::stdem_error_string(error));
        }
        return result;
    }
    
    /**
     * @brief Array-style access
     */
    template<typename T>
    const T& operator[](int enum_value) const {
        return get<T>(enum_value);
    }
    
    /**
     * @brief Returns raw C handle
     */
    ::EnumMap* c_handle() const { return map_; }
    
    /**
     * @brief Checks if map is empty
     */
    bool empty() const { return size() == 0; }
};

/**
 * @brief Creates EnumMap from initializer list
 */
template<typename T>
EnumMap make_enum_map(std::initializer_list<std::pair<int, T>> init_list,
                     StemFlags flags = STEM_FLAGS_COPY_VALUES) {
    return EnumMap(init_list, sizeof(T), flags);
}

/**
 * @brief Creates EnumMap for pointer types
 */
EnumMap make_pointer_map(std::initializer_list<std::pair<int, void*>> init_list,
                        StemFlags flags = STEM_FLAGS_NONE) {
    return EnumMap(init_list, 0, flags);
}

} // namespace stem

#endif // __cplusplus

#endif /* STDEM_H */