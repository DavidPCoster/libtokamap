#!/bin/bash

# LibTokaMap Packaging Script
# This script builds and packages libtokamap for distribution

set -e  # Exit on error

# Default values
BUILD_TYPE="Release"
BUILD_DIR="build-release"
PACKAGE_TYPES=""
CLEAN_BUILD=false
VERBOSE=false

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Function to show usage
show_usage() {
    cat << EOF
Usage: $0 [OPTIONS]

Options:
    -t, --type TYPE         Package type(s): deb, rpm, tgz, zip, productbuild (comma-separated)
    -b, --build-type TYPE   Build type: Debug, Release, RelWithDebInfo (default: Release)
    -d, --build-dir DIR     Build directory (default: build-release)
    -c, --clean             Clean build directory before building
    -v, --verbose           Verbose output
    -h, --help              Show this help message

Examples:
    $0                      # Build and create all available packages
    $0 -t deb,rpm          # Create only DEB and RPM packages
    $0 -b Debug -c         # Clean build in Debug mode
    $0 -t tgz -v           # Create TGZ package with verbose output

Available package types depend on platform:
    Linux:   deb, rpm, tgz, zip
    macOS:   tgz, zip, productbuild
    Windows: zip, nsis (if available)

EOF
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -t|--type)
            PACKAGE_TYPES="$2"
            shift 2
            ;;
        -b|--build-type)
            BUILD_TYPE="$2"
            shift 2
            ;;
        -d|--build-dir)
            BUILD_DIR="$2"
            shift 2
            ;;
        -c|--clean)
            CLEAN_BUILD=true
            shift
            ;;
        -v|--verbose)
            VERBOSE=true
            shift
            ;;
        -h|--help)
            show_usage
            exit 0
            ;;
        *)
            print_error "Unknown option: $1"
            show_usage
            exit 1
            ;;
    esac
done

# Get script directory and project root
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

print_status "LibTokaMap Packaging Script"
print_status "Project directory: $PROJECT_DIR"
print_status "Build type: $BUILD_TYPE"
print_status "Build directory: $BUILD_DIR"

# Detect platform
if [[ "$OSTYPE" == "darwin"* ]]; then
    PLATFORM="macOS"
    DEFAULT_GENERATORS="TGZ;ZIP"
elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
    PLATFORM="Linux"
    DEFAULT_GENERATORS="DEB;RPM;TGZ"
elif [[ "$OSTYPE" == "msys" || "$OSTYPE" == "win32" ]]; then
    PLATFORM="Windows"
    DEFAULT_GENERATORS="ZIP"
else
    PLATFORM="Unknown"
    DEFAULT_GENERATORS="TGZ"
fi

print_status "Detected platform: $PLATFORM"

# Set package generators based on user input or platform defaults
if [[ -n "$PACKAGE_TYPES" ]]; then
    # Convert comma-separated list to CMake generator format
    GENERATORS=""
    IFS=',' read -ra TYPES <<< "$PACKAGE_TYPES"
    for type in "${TYPES[@]}"; do
        case $type in
            deb) GENERATORS="${GENERATORS}DEB;" ;;
            rpm) GENERATORS="${GENERATORS}RPM;" ;;
            tgz) GENERATORS="${GENERATORS}TGZ;" ;;
            zip) GENERATORS="${GENERATORS}ZIP;" ;;
            productbuild) GENERATORS="${GENERATORS}productbuild;" ;;
            *)
                print_warning "Unknown package type: $type"
                ;;
        esac
    done
    GENERATORS="${GENERATORS%;}"  # Remove trailing semicolon
else
    GENERATORS="$DEFAULT_GENERATORS"
fi

print_status "Package generators: $GENERATORS"

# Change to project directory
cd "$PROJECT_DIR"

# Clean build directory if requested
if [[ "$CLEAN_BUILD" == true ]]; then
    print_status "Cleaning build directory..."
    rm -rf "$BUILD_DIR"
fi

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure with CMake
print_status "Configuring CMake..."
CMAKE_ARGS=(
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
    -DCPACK_GENERATOR="$GENERATORS"
    -DENABLE_TESTING=OFF
    -DENABLE_EXAMPLES=OFF
)

if [[ "$VERBOSE" == true ]]; then
    CMAKE_ARGS+=(-DCMAKE_VERBOSE_MAKEFILE=ON)
fi

cmake "${CMAKE_ARGS[@]}" ..

# Build the project
print_status "Building project..."
if [[ "$VERBOSE" == true ]]; then
    cmake --build . --config "$BUILD_TYPE" -- VERBOSE=1
else
    cmake --build . --config "$BUILD_TYPE"
fi

# Create packages
print_status "Creating packages..."
if [[ "$VERBOSE" == true ]]; then
    cpack --verbose
else
    cpack
fi

# List created packages
print_status "Package creation completed!"
echo ""
echo "Created packages:"
for package in *.deb *.rpm *.tar.gz *.zip *.pkg 2>/dev/null; do
    if [[ -f "$package" ]]; then
        echo "  - $package ($(du -h "$package" | cut -f1))"
    fi
done

# Show package location
echo ""
print_status "Packages are located in: $(pwd)"

# Verify packages (basic checks)
print_status "Performing basic package verification..."

if ls *.deb 1> /dev/null 2>&1; then
    for deb in *.deb; do
        if command -v dpkg >/dev/null 2>&1; then
            print_status "Verifying DEB package: $deb"
            dpkg --info "$deb" | head -20
        fi
    done
fi

if ls *.rpm 1> /dev/null 2>&1; then
    for rpm in *.rpm; do
        if command -v rpm >/dev/null 2>&1; then
            print_status "Verifying RPM package: $rpm"
            rpm -qip "$rpm" | head -20
        fi
    done
fi

print_status "Packaging process completed successfully!"
