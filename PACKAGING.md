# LibTokaMap Packaging Guide

This document describes how to create distribution packages for LibTokaMap on Linux and macOS.

## Overview

LibTokaMap includes comprehensive CMake-based packaging support using CPack. The packaging system supports multiple formats and platforms:

- **Linux**: DEB (Debian/Ubuntu), RPM (Red Hat/SUSE), and TGZ archives
- **macOS**: TGZ archives, ZIP files, and optionally ProductBuild packages
- **Cross-platform**: Source packages in TGZ and ZIP formats

## Quick Start

The easiest way to create packages is using the provided packaging script:

```bash
# Create all available packages for your platform
./scripts/package.sh

# Create specific package types
./scripts/package.sh -t deb,rpm

# Clean build and create packages with verbose output
./scripts/package.sh -c -v
```

## Manual Packaging

If you prefer to build packages manually:

```bash
# Configure and build
mkdir build-release
cd build-release
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .

# Create packages
cpack
```

## Package Types

### Linux Packages

#### DEB Packages (Debian/Ubuntu)
- **Format**: `.deb`
- **Target**: Debian, Ubuntu, and derivatives
- **Features**: Automatic dependency resolution, integration with apt
- **Architecture**: Automatically detected (amd64, i386)

#### RPM Packages (Red Hat/SUSE)
- **Format**: `.rpm`
- **Target**: Red Hat, SUSE, CentOS, Fedora
- **Features**: Automatic dependency resolution, integration with yum/dnf/zypper
- **Architecture**: Automatically detected (x86_64, i386)

### macOS Packages

#### TGZ Archives
- **Format**: `.tar.gz`
- **Target**: macOS (all versions)
- **Features**: Universal format, Homebrew compatible
- **Use case**: Manual installation, CI/CD pipelines

#### ZIP Archives
- **Format**: `.zip`
- **Target**: macOS (all versions)
- **Features**: Native macOS archive format
- **Use case**: End-user distribution

#### ProductBuild (Optional)
- **Format**: `.pkg`
- **Target**: macOS with installer
- **Features**: Native macOS installer, can be signed and notarized
- **Requirements**: Apple Developer certificate

### Source Packages

Source packages include all source code and build scripts:
- **Formats**: `.tar.gz`, `.zip`
- **Contents**: Source code, CMake files, documentation
- **Excludes**: Build artifacts, IDE files, version control

## Package Contents

All packages include the following components:

### Runtime Component
- Shared library (`libtokamap.so` or `libtokamap.dylib`)
- License and version information

### Development Component
- Headers (`libtokamap.hpp`, `version.hpp`)
- CMake configuration files
- Documentation (README, LICENSE)

## Customizing Packages

### Package Metadata

Edit the CMakeLists.txt file to customize package information:

```cmake
configure_basic_packaging(
    NAME "libtokamap"
    VERSION ${PROJECT_VERSION}
    DESCRIPTION "Your custom description"
    CONTACT "your-email@domain.com"
    URL "https://your-website.com"
    DEPENDENCIES_DEB "your-deps"
    DEPENDENCIES_RPM "your-deps"
)
```

### Adding Components

To add custom components:

```cmake
configure_components(
    RUNTIME_COMPONENTS "your-runtime-component"
    DEVELOPMENT_COMPONENTS "your-dev-component"
)
```

### Custom Ignore Patterns

For source packages, add custom ignore patterns:

```cmake
configure_source_packaging(
    IGNORE_PATTERNS "/your-custom-path/" "*.tmp"
)
```

## Dependencies

### Build Dependencies
- CMake 3.15 or newer
- C++20 compatible compiler (GCC 9+, Clang 10+, Apple Clang 12+)

### Runtime Dependencies

#### Linux
- `glibc >= 2.17`
- `libstdc++ >= 9`

#### macOS
- macOS 10.15 (Catalina) or newer
- System libraries (automatically linked)

### Package Creation Dependencies

#### Linux
- `dpkg-dev` (for DEB packages)
- `rpm-build` (for RPM packages)

#### macOS
- Xcode Command Line Tools
- Optional: Apple Developer certificate (for signed packages)

## Packaging Script Options

The `scripts/package.sh` script supports various options:

```bash
Usage: package.sh [OPTIONS]

Options:
    -t, --type TYPE         Package type(s): deb, rpm, tgz, zip, productbuild
    -b, --build-type TYPE   Build type: Debug, Release, RelWithDebInfo
    -d, --build-dir DIR     Build directory (default: build-release)
    -c, --clean             Clean build directory before building
    -v, --verbose           Verbose output
    -h, --help              Show help message
```

### Examples

```bash
# Create DEB and RPM packages only
./scripts/package.sh -t deb,rpm

# Debug build with verbose output
./scripts/package.sh -b Debug -v

# Custom build directory
./scripts/package.sh -d my-build-dir

# Clean build in release mode
./scripts/package.sh -c -b Release
```

## Installation

### From DEB Package
```bash
sudo dpkg -i libtokamap_*.deb
sudo apt-get install -f  # Fix dependencies if needed
```

### From RPM Package
```bash
sudo rpm -ivh libtokamap-*.rpm
# or
sudo yum install libtokamap-*.rpm
# or
sudo dnf install libtokamap-*.rpm
```

### From TGZ/ZIP Archive
```bash
# Extract
tar -xzf libtokamap-*.tar.gz  # for TGZ
unzip libtokamap-*.zip        # for ZIP

# Manual installation
sudo cp -r include/* /usr/local/include/
sudo cp lib/* /usr/local/lib/
sudo ldconfig  # Linux only
```

### Using CMake
After installation, use LibTokaMap in your CMake projects:

```cmake
find_package(LibTokaMap REQUIRED)
target_link_libraries(your_target LibTokaMap::libtokamap)
```

## Troubleshooting

### Common Issues

#### Missing Dependencies
- **DEB**: Use `apt-get install -f` to fix dependencies
- **RPM**: Ensure all dependencies are available in your repositories

#### Architecture Mismatch
- Verify you're building on the target architecture
- Check CMake output for architecture detection

#### Permission Issues
- Use `sudo` for system-wide installation
- Consider user-local installation in `~/.local/`

#### CMake Configuration Issues
- Ensure CMake 3.15+ is installed
- Check that all required CMake modules are available

### Debug Information

Enable verbose output to debug packaging issues:

```bash
./scripts/package.sh -v
```

Or manually:

```bash
cmake --build . --verbose
cpack --verbose
```

### Platform-Specific Issues

#### Linux
- Install development packages: `build-essential`, `cmake`
- For cross-compilation, set appropriate CMake toolchain

#### macOS
- Install Xcode Command Line Tools
- For universal binaries, configure multiple architectures in CMake

## Contributing

When adding new packaging features:

1. Update `cmake/PackagingUtils.cmake` for reusable functionality
2. Test on target platforms
3. Update this documentation
4. Add appropriate error handling

## Support

For packaging-related issues:

1. Check this documentation
2. Review CMake and CPack documentation
3. Check platform-specific package manager documentation
4. File an issue with detailed build logs