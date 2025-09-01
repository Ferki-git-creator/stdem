/**
 * @file test_stdem.c
 * @brief Comprehensive test suite for Standard Enum Mapping Library
 * @author Ferki
 * @license LGPL-3.0-or-later
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../include/stdem.h"

/* ==================== TEST DEFINES AND STRUCTURES ==================== */

#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            fprintf(stderr, "TEST FAILED: %s (%s:%d)\n", message, __FILE__, __LINE__); \
            return 1; \
        } \
    } while (0)

#define TEST_RUN(test_function) \
    do { \
        printf("Running %s... ", #test_function); \
        int result = test_function(); \
        if (result != 0) { \
            printf("FAILED\n"); \
            failures++; \
        } else { \
            printf("PASSED\n"); \
            passed++; \
        } \
        total++; \
    } while (0)

typedef struct {
    int enum_value;
    const char* name;
    int data;
} TestEntry;

/* ==================== TEST FIXTURES ==================== */

static const TestEntry test_entries[] = {
    {1, "STATE_IDLE", 100},
    {2, "STATE_ACTIVE", 200},
    {3, "STATE_ERROR", 300},
    {4, "STATE_SHUTDOWN", 400},
    {5, "STATE_INIT", 500}
};

static const size_t test_entries_count = sizeof(test_entries) / sizeof(test_entries[0]);

// Global iterator function for testing
static void test_iterator(int enum_value, const char* name, const void* value, 
                         size_t value_size, void* user_data) {
    (void)value_size; // Mark parameter as used
    size_t* count_ptr = (size_t*)user_data;
    (*count_ptr)++;
    
    // Verify that we can find this entry in our test data
    int found = 0;
    for (size_t i = 0; i < test_entries_count; i++) {
        if (test_entries[i].enum_value == enum_value) {
            assert(strcmp(test_entries[i].name, name) == 0);
            assert(*(const int*)value == test_entries[i].data);
            found = 1;
            break;
        }
    }
    assert(found);
}

/* ==================== TEST CASES ==================== */

/**
 * @brief Test basic map creation and destruction
 */
static int test_create_destroy(void) {
    StemError error;
    EnumMap* map = stdem_create_ex(10, sizeof(int), STEM_FLAGS_NONE, &error);
    TEST_ASSERT(map != NULL, "Map creation failed");
    TEST_ASSERT(error == STEM_SUCCESS, "Error code should be success");
    TEST_ASSERT(stdem_count(map) == 0, "New map should be empty");
    
    stdem_destroy(map);
    return 0;
}

/**
 * @brief Test value association and retrieval
 */
static int test_associate_get(void) {
    StemError error;
    EnumMap* map = stdem_create_ex(test_entries_count, sizeof(int), STEM_FLAGS_NONE, &error);
    TEST_ASSERT(map != NULL, "Map creation failed");
    
    // Associate values
    for (size_t i = 0; i < test_entries_count; i++) {
        error = stdem_associate_ex(map, test_entries[i].enum_value, 
                                 &test_entries[i].data, test_entries[i].name);
        TEST_ASSERT(error == STEM_SUCCESS, "Association should succeed");
    }
    
    TEST_ASSERT(stdem_count(map) == test_entries_count, "Map should contain all entries");
    
    // Retrieve and verify values
    for (size_t i = 0; i < test_entries_count; i++) {
        const int* value = stdem_get_value_ex(map, test_entries[i].enum_value, &error);
        TEST_ASSERT(value != NULL, "Value should be found");
        TEST_ASSERT(error == STEM_SUCCESS, "Error code should be success");
        TEST_ASSERT(*value == test_entries[i].data, "Retrieved value should match");
        
        const char* name = stdem_get_name_ex(map, test_entries[i].enum_value, &error);
        TEST_ASSERT(name != NULL, "Name should be found");
        TEST_ASSERT(error == STEM_SUCCESS, "Error code should be success");
        TEST_ASSERT(strcmp(name, test_entries[i].name) == 0, "Retrieved name should match");
    }
    
    stdem_destroy(map);
    return 0;
}

/**
 * @brief Test error handling for invalid operations
 */
static int test_error_handling(void) {
    StemError error;
    
    // Test NULL map
    const void* value = stdem_get_value_ex(NULL, 1, &error);
    TEST_ASSERT(value == NULL, "Should return NULL for NULL map");
    TEST_ASSERT(error == STEM_ERROR_INVALID_ARG, "Should return invalid argument error");
    
    // Test invalid enum value
    EnumMap* map = stdem_create_ex(5, sizeof(int), STEM_FLAGS_NONE, &error);
    value = stdem_get_value_ex(map, 999, &error);
    TEST_ASSERT(value == NULL, "Should return NULL for non-existent value");
    TEST_ASSERT(error == STEM_ERROR_NOT_FOUND, "Should return not found error");
    
    // Test duplicate association
    int data = 42;
    error = stdem_associate_ex(map, 1, &data, "TEST");
    TEST_ASSERT(error == STEM_SUCCESS, "First association should succeed");
    
    error = stdem_associate_ex(map, 1, &data, "TEST_DUPLICATE");
    TEST_ASSERT(error == STEM_ERROR_ALREADY_EXISTS, "Should return already exists error");
    
    stdem_destroy(map);
    return 0;
}

/**
 * @brief Test map iteration functionality
 */
static int test_iteration(void) {
    StemError error;
    EnumMap* map = stdem_create_ex(test_entries_count, sizeof(int), STEM_FLAGS_NONE, &error);
    TEST_ASSERT(map != NULL, "Map creation failed");
    
    // Populate map
    for (size_t i = 0; i < test_entries_count; i++) {
        error = stdem_associate_ex(map, test_entries[i].enum_value, 
                                 &test_entries[i].data, test_entries[i].name);
        TEST_ASSERT(error == STEM_SUCCESS, "Association should succeed");
    }
    
    // Test iteration
    size_t count = 0;
    error = stdem_foreach(map, test_iterator, &count);
    TEST_ASSERT(error == STEM_SUCCESS, "Iteration should succeed");
    TEST_ASSERT(count == test_entries_count, "Should iterate over all entries");
    
    stdem_destroy(map);
    return 0;
}

/**
 * @brief Test map copying functionality
 */
static int test_copy(void) {
    StemError error;
    EnumMap* map = stdem_create_ex(test_entries_count, sizeof(int), STEM_FLAGS_NONE, &error);
    TEST_ASSERT(map != NULL, "Map creation failed");
    
    // Populate map
    for (size_t i = 0; i < test_entries_count; i++) {
        error = stdem_associate_ex(map, test_entries[i].enum_value, 
                                 &test_entries[i].data, test_entries[i].name);
        TEST_ASSERT(error == STEM_SUCCESS, "Association should succeed");
    }
    
    // Create copy
    EnumMap* copy = stdem_copy(map, &error);
    TEST_ASSERT(copy != NULL, "Copy should succeed");
    TEST_ASSERT(error == STEM_SUCCESS, "Error code should be success");
    TEST_ASSERT(stdem_count(copy) == test_entries_count, "Copy should have same number of entries");
    
    // Verify copied values
    for (size_t i = 0; i < test_entries_count; i++) {
        const int* value = stdem_get_value_ex(copy, test_entries[i].enum_value, &error);
        TEST_ASSERT(value != NULL, "Value should be found in copy");
        TEST_ASSERT(*value == test_entries[i].data, "Copied value should match");
        
        const char* name = stdem_get_name_ex(copy, test_entries[i].enum_value, &error);
        TEST_ASSERT(name != NULL, "Name should be found in copy");
        TEST_ASSERT(strcmp(name, test_entries[i].name) == 0, "Copied name should match");
    }
    
    stdem_destroy(map);
    stdem_destroy(copy);
    return 0;
}

/**
 * @brief Test map merging functionality
 */
static int test_merge(void) {
    StemError error;
    
    // Create first map
    EnumMap* map1 = stdem_create_ex(3, sizeof(int), STEM_FLAGS_NONE, &error);
    TEST_ASSERT(map1 != NULL, "Map creation failed");
    
    int data1[] = {100, 200, 300};
    error = stdem_associate_ex(map1, 1, &data1[0], "ENTRY_1");
    TEST_ASSERT(error == STEM_SUCCESS, "Association should succeed");
    error = stdem_associate_ex(map1, 2, &data1[1], "ENTRY_2");
    TEST_ASSERT(error == STEM_SUCCESS, "Association should succeed");
    error = stdem_associate_ex(map1, 3, &data1[2], "ENTRY_3");
    TEST_ASSERT(error == STEM_SUCCESS, "Association should succeed");
    
    // Create second map
    EnumMap* map2 = stdem_create_ex(3, sizeof(int), STEM_FLAGS_NONE, &error);
    TEST_ASSERT(map2 != NULL, "Map creation failed");
    
    int data2[] = {400, 500, 600};
    error = stdem_associate_ex(map2, 3, &data2[0], "ENTRY_3_NEW"); // Overlapping
    TEST_ASSERT(error == STEM_SUCCESS, "Association should succeed");
    error = stdem_associate_ex(map2, 4, &data2[1], "ENTRY_4");
    TEST_ASSERT(error == STEM_SUCCESS, "Association should succeed");
    error = stdem_associate_ex(map2, 5, &data2[2], "ENTRY_5");
    TEST_ASSERT(error == STEM_SUCCESS, "Association should succeed");
    
    // Test merge without overwrite
    EnumMap* merged = stdem_merge(map1, map2, 0, &error);
    TEST_ASSERT(merged != NULL, "Merge should succeed");
    TEST_ASSERT(stdem_count(merged) == 5, "Merged map should have 5 entries");
    
    // Verify non-overwrite behavior
    const int* value = stdem_get_value_ex(merged, 3, &error);
    TEST_ASSERT(value != NULL, "Value should be found");
    TEST_ASSERT(*value == data1[2], "Should keep original value when not overwriting");
    
    stdem_destroy(merged);
    
    // Test merge with overwrite
    merged = stdem_merge(map1, map2, 1, &error);
    TEST_ASSERT(merged != NULL, "Merge should succeed");
    TEST_ASSERT(stdem_count(merged) == 5, "Merged map should have 5 entries");
    
    // Verify overwrite behavior
    value = stdem_get_value_ex(merged, 3, &error);
    TEST_ASSERT(value != NULL, "Value should be found");
    TEST_ASSERT(*value == data2[0], "Should use new value when overwriting");
    
    stdem_destroy(map1);
    stdem_destroy(map2);
    stdem_destroy(merged);
    return 0;
}

/**
 * @brief Test clear functionality
 */
static int test_clear(void) {
    StemError error;
    EnumMap* map = stdem_create_ex(test_entries_count, sizeof(int), STEM_FLAGS_NONE, &error);
    TEST_ASSERT(map != NULL, "Map creation failed");
    
    // Populate map
    for (size_t i = 0; i < test_entries_count; i++) {
        error = stdem_associate_ex(map, test_entries[i].enum_value, 
                                 &test_entries[i].data, test_entries[i].name);
        TEST_ASSERT(error == STEM_SUCCESS, "Association should succeed");
    }
    
    TEST_ASSERT(stdem_count(map) == test_entries_count, "Map should contain entries");
    
    // Clear map
    error = stdem_clear(map);
    TEST_ASSERT(error == STEM_SUCCESS, "Clear should succeed");
    TEST_ASSERT(stdem_count(map) == 0, "Map should be empty after clear");
    
    // Verify values are gone
    for (size_t i = 0; i < test_entries_count; i++) {
        const int* value = stdem_get_value_ex(map, test_entries[i].enum_value, &error);
        TEST_ASSERT(value == NULL, "Values should be removed after clear");
        TEST_ASSERT(error == STEM_ERROR_NOT_FOUND, "Should return not found error");
    }
    
    stdem_destroy(map);
    return 0;
}

/**
 * @brief Test flags functionality
 */
static int test_flags(void) {
    StemError error;
    
    // Test NO_NAMES flag
    EnumMap* map = stdem_create_ex(test_entries_count, sizeof(int), STEM_FLAGS_NO_NAMES, &error);
    TEST_ASSERT(map != NULL, "Map creation failed");
    
    error = stdem_associate_ex(map, 1, &test_entries[0].data, test_entries[0].name);
    TEST_ASSERT(error == STEM_SUCCESS, "Association should succeed");
    
    const char* name = stdem_get_name_ex(map, 1, &error);
    TEST_ASSERT(name == NULL, "Name should not be stored with NO_NAMES flag");
    TEST_ASSERT(error == STEM_ERROR_NOT_FOUND, "Should return not found error");
    
    stdem_destroy(map);
    
    // Test COPY_VALUES flag
    map = stdem_create_ex(1, sizeof(int), STEM_FLAGS_COPY_VALUES, &error);
    TEST_ASSERT(map != NULL, "Map creation failed");
    
    int original_value = 42;
    error = stdem_associate_ex(map, 1, &original_value, "TEST");
    TEST_ASSERT(error == STEM_SUCCESS, "Association should succeed");
    
    // Modify original value - should not affect stored value
    original_value = 100;
    const int* stored_value = stdem_get_value_ex(map, 1, &error);
    TEST_ASSERT(stored_value != NULL, "Value should be found");
    TEST_ASSERT(*stored_value == 42, "Stored value should not be affected by original modification");
    
    stdem_destroy(map);
    return 0;
}

/**
 * @brief Test find by name functionality
 */
static int test_find_by_name(void) {
    StemError error;
    EnumMap* map = stdem_create_ex(test_entries_count, sizeof(int), STEM_FLAGS_NONE, &error);
    TEST_ASSERT(map != NULL, "Map creation failed");
    
    // Populate map
    for (size_t i = 0; i < test_entries_count; i++) {
        error = stdem_associate_ex(map, test_entries[i].enum_value, 
                                 &test_entries[i].data, test_entries[i].name);
        TEST_ASSERT(error == STEM_SUCCESS, "Association should succeed");
    }
    
    // Test finding by name
    for (size_t i = 0; i < test_entries_count; i++) {
        int found_value = stdem_find_by_name(map, test_entries[i].name, &error);
        TEST_ASSERT(found_value == test_entries[i].enum_value, "Should find correct enum value by name");
        TEST_ASSERT(error == STEM_SUCCESS, "Error code should be success");
    }
    
    // Test non-existent name
    int found_value = stdem_find_by_name(map, "NON_EXISTENT_NAME", &error);
    TEST_ASSERT(found_value == 0, "Should return 0 for non-existent name");
    TEST_ASSERT(error == STEM_ERROR_NOT_FOUND, "Should return not found error");
    
    stdem_destroy(map);
    return 0;
}

/* ==================== TEST RUNNER ==================== */

int main(void) {
    printf("Starting Standard Enum Mapping Library tests...\n\n");
    
    int passed = 0;
    int failures = 0;
    int total = 0;
    
    // Run all test cases
    TEST_RUN(test_create_destroy);
    TEST_RUN(test_associate_get);
    TEST_RUN(test_error_handling);
    TEST_RUN(test_iteration);
    TEST_RUN(test_copy);
    TEST_RUN(test_merge);
    TEST_RUN(test_clear);
    TEST_RUN(test_flags);
    TEST_RUN(test_find_by_name);
    
    printf("\nTest Results: %d passed, %d failed, %d total\n", passed, failures, total);
    
    if (failures == 0) {
        printf("All tests PASSED! ✅\n");
        return 0;
    } else {
        printf("Some tests FAILED! ❌\n");
        return 1;
    }
}