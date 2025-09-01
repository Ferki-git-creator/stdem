Installation Guide - Standard Enum Mapping Library

Prerequisites

Before installing the Standard Enum Mapping Library, ensure your system has:

· A C99-compatible compiler (GCC, Clang, MSVC, etc.)
· GNU Make (for building from source)
· Doxygen (optional, for generating documentation)

Installation Methods

Method 1: Building from Source (Recommended)

Linux/macOS

1. Clone or download the library
   ```bash
   git clone 
https://github.com/Ferki-git-creator/stdem
   cd stdem
   ```
2. Build the library
   ```bash
   make static
   ```
3. Run tests to verify the build
   ```bash
   make tests
   ```
4. Install system-wide
   ```bash
   sudo make install
   ```
   This will install the header file to /usr/local/include and the library to /usr/local/lib
5. (Optional) Generate documentation
   ```bash
   make docs
   ```

Windows

1. Install MinGW-w64 or MSYS2
   · Download from https://www.mingw-w64.org/ or https://www.msys2.org/
2. Open MinGW or MSYS2 terminal
3. Navigate to the library directory
4. Build the library
   ```bash
   make static
   ```
5. Run tests
   ```bash
   make tests
   ```
6. Install (optional)
   ```bash
   make install PREFIX=/mingw64
   ```

Method 2: Using in Your Project

Direct Inclusion

1. Copy the header and source files to your project:
   ```bash
   cp include/stdem.h your-project/include/
   cp src/stdem.c your-project/src/
   ```
2. Add to your compilation:
   ```bash
   gcc -Iyour-project/include -c your-project/src/stdem.c -o stdem.o
   ```
3. Link with your application:
   ```bash
   gcc your-app.c stdem.o -o your-app
   ```

As a Static Library

1. Build the library:
   ```bash
   make static
   ```
2. Copy the built library to your project:
   ```bash
   cp build/libstdem.a your-project/libs/
   cp include/stdem.h your-project/include/
   ```
3. Link with your application:
   ```bash
   gcc -Iyour-project/include your-app.c -Lyour-project/libs -lstdem -o your-app
   ```

Method 3: Package Manager Installation

Note: Package manager support is planned for future releases.

Verification

After installation, verify the library works correctly:

1. Create a test program (test_install.c):
   ```c
   #include <stdio.h>
   #include <stdem.h>
   
   int main() {
       StemError error;
       EnumMap* map = stdem_create(5, sizeof(int), STEM_FLAGS_NONE, &error);
       
       if (map) {
           printf("Library installed successfully!\n");
           stdem_destroy(map);
           return 0;
       } else {
           printf("Installation failed: %s\n", stdem_error_string(error));
           return 1;
       }
   }
   ```
2. Compile and run:
   ```bash
   gcc test_install.c -lstdem -o test_install
   ./test_install
   ```

Custom Installation Path

To install to a custom location:

```bash
make install PREFIX=/your/custom/path
```

This will install:

· Header file: /your/custom/path/include/stdem.h
· Library: /your/custom/path/lib/libstdem.a

When using a custom install path, update your compilation flags:

```bash
gcc -I/your/custom/path/include your-app.c -L/your/custom/path/lib -lstdem -o your-app
```

Uninstallation

To remove the library from your system:

```bash
sudo make uninstall
```

For custom installation paths:

```bash
make uninstall PREFIX=/your/custom/path
```

Platform-Specific Notes

Linux

· The library is compatible with all major Linux distributions
· No additional dependencies required

macOS

· Compatible with macOS 10.9 and later
· Use Homebrew to install dependencies if needed:
  ```bash
  brew install make doxygen
  ```

Windows

· MinGW-w64 or MSYS2 required for building
· For Visual Studio integration, add the library to your project manually

Embedded Systems

· The library has zero dependencies and minimal footprint
· Perfect for ARM Cortex-M, AVR, ESP32, and other embedded platforms
· Simply include the source files in your embedded project

Troubleshooting

Common Issues

1. "stdem.h not found" error
   · Ensure the include path is correct with -I/path/to/include
2. "libstdem.a not found" error
   · Ensure the library path is correct with -L/path/to/lib
3. Linker errors
   · Make sure you're linking with -lstdem
4. Doxygen not found
   · Install Doxygen or skip documentation generation

Support

If you encounter issues:

1. Check the GitHub Issues
2. Consult the API Documentation
3. Review the Usage Examples

License Notice

This library is distributed under the LGPL-3.0-or-later license. By using this software, you agree to the terms of this license.