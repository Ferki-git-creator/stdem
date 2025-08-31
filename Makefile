# Standard Enum Mapping Library Makefile
# Author: Ferki
# License: LGPL-3.0-or-later

# ==================== CONFIGURATION ====================

# Compiler and tools
CC = gcc
AR = ar
MKDIR = mkdir -p
RM = rm -rf
DOXYGEN = doxygen

# Project structure
SRC_DIR = src
INCLUDE_DIR = include
TESTS_DIR = tests
BUILD_DIR = build
DOCS_DIR = docs

# Target names
LIB_NAME = stdem
STATIC_LIB = lib$(LIB_NAME).a
SHARED_LIB = lib$(LIB_NAME).so
TEST_BIN = test_$(LIB_NAME)

# Installation directories
PREFIX ?= /usr/local
INCLUDE_INSTALL_DIR = $(PREFIX)/include
LIB_INSTALL_DIR = $(PREFIX)/lib

# Compiler flags
CFLAGS = -I$(INCLUDE_DIR) -std=c99 -Wall -Wextra -pedantic -fPIC
# Debug flags
DEBUG_CFLAGS = -g -O0 -DDEBUG
# Release flags
RELEASE_CFLAGS = -O3 -DNDEBUG

# Linker flags
LDFLAGS = -L$(BUILD_DIR) -l$(LIB_NAME)

# Source files
SRC_FILES = $(SRC_DIR)/stdem.c
OBJ_FILES = $(SRC_FILES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

# Test files
TEST_SRC = $(TESTS_DIR)/test_stdem.c
TEST_OBJ = $(BUILD_DIR)/test_stdem.o

# ==================== BUILD TARGETS ====================

# Default build: static library
all: static

# Build static library
static: CFLAGS += $(RELEASE_CFLAGS)
static: $(BUILD_DIR)/$(STATIC_LIB)

# Build shared library
shared: CFLAGS += $(RELEASE_CFLAGS)
shared: $(BUILD_DIR)/$(SHARED_LIB)

# Build static library with debug info
debug: CFLAGS += $(DEBUG_CFLAGS)
debug: $(BUILD_DIR)/$(STATIC_LIB)

# Build everything
everything: static shared tests

# Create build directory
$(BUILD_DIR):
	$(MKDIR) $(BUILD_DIR)

# Compile source files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Build static library
$(BUILD_DIR)/$(STATIC_LIB): $(OBJ_FILES) | $(BUILD_DIR)
	$(AR) rcs $@ $^

# Build shared library
$(BUILD_DIR)/$(SHARED_LIB): $(OBJ_FILES) | $(BUILD_DIR)
	$(CC) -shared $^ -o $@

# ==================== TEST TARGETS ====================

# Build and run tests
tests: CFLAGS += $(DEBUG_CFLAGS)
tests: $(BUILD_DIR)/$(TEST_BIN)
	@echo "Running tests..."
	@./$(BUILD_DIR)/$(TEST_BIN)

# Build test executable
$(BUILD_DIR)/$(TEST_BIN): $(TEST_OBJ) $(BUILD_DIR)/$(STATIC_LIB)
	$(CC) $(TEST_OBJ) -o $@ $(LDFLAGS)

# Compile test source
$(BUILD_DIR)/test_stdem.o: $(TEST_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# ==================== INSTALL TARGETS ====================

# Install library and headers
install: static
	$(MKDIR) $(DESTDIR)$(INCLUDE_INSTALL_DIR)
	$(MKDIR) $(DESTDIR)$(LIB_INSTALL_DIR)
	install -m 644 $(INCLUDE_DIR)/stdem.h $(DESTDIR)$(INCLUDE_INSTALL_DIR)
	install -m 644 $(BUILD_DIR)/$(STATIC_LIB) $(DESTDIR)$(LIB_INSTALL_DIR)
	@echo "Library installed successfully!"

# Uninstall library and headers
uninstall:
	$(RM) $(DESTDIR)$(INCLUDE_INSTALL_DIR)/stdem.h
	$(RM) $(DESTDIR)$(LIB_INSTALL_DIR)/$(STATIC_LIB)
	@echo "Library uninstalled successfully!"

# ==================== DOCUMENTATION TARGETS ====================

# Generate documentation
docs:
	@if command -v $(DOXYGEN) >/dev/null 2>&1; then \
		$(DOXYGEN) $(DOCS_DIR)/Doxyfile; \
		echo "Documentation generated in $(DOCS_DIR)/html"; \
	else \
		echo "Error: doxygen is not installed. Please install it to generate documentation."; \
		exit 1; \
	fi

# ==================== CLEAN TARGETS ====================

# Clean build artifacts
clean:
	$(RM) $(BUILD_DIR)
	@echo "Build artifacts cleaned!"

# Clean everything including documentation
distclean: clean
	$(RM) $(DOCS_DIR)/html $(DOCS_DIR)/latex
	@echo "All generated files cleaned!"

# ==================== PHONY TARGETS ====================

.PHONY: all static shared debug everything tests install uninstall docs clean distclean

# ==================== USAGE TARGET ====================

# Show usage information
usage:
	@echo "Standard Enum Mapping Library - Build System"
	@echo ""
	@echo "Available targets:"
	@echo "  make all           # Build static library (default)"
	@echo "  make static        # Build static library (release)"
	@echo "  make shared        # Build shared library"
	@echo "  make debug         # Build with debug information"
	@echo "  make tests         # Build and run tests"
	@echo "  make docs          # Generate documentation (requires Doxygen)"
	@echo "  make install       # Install library and headers"
	@echo "  make uninstall     # Uninstall library and headers"
	@echo "  make clean         # Clean build artifacts"
	@echo "  make distclean     # Clean all generated files"
	@echo ""
	@echo "Examples:"
	@echo "  make              # Build static library"
	@echo "  make tests        # Run tests"
	@echo "  make install      # Install to system"