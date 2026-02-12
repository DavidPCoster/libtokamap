# LibTokaMap Documentation

Welcome to the LibTokaMap documentation! LibTokaMap is a C++20 library for flexible data mapping and transformation with support for multiple data sources and mapping types.

## What is LibTokaMap?

LibTokaMap provides a plugin-based architecture for data transformation and mapping operations. It allows you to define complex data transformations through JSON-based configuration files and supports various data sources and mapping types.

### Key Features

- **Multiple Mapping Types**: Value, data source, expression, dimension, and custom mappings
- **Pluggable Data Sources**: Extensible architecture for custom data source implementations
- **JSON Configuration**: Human-readable JSON-based mapping definitions
- **Caching Support**: Built-in RAM caching for improved performance
- **Type Safety**: Strong typing with C++20 features
- **Subset Operations**: Advanced data slicing and subsetting capabilities
- **Scale/Offset Transformations**: Built-in support for linear data transformations

## Quick Start

```cpp
#include <libtokamap.hpp>

int main() {
    libtokamap::MappingHandler mapping_handler;

    // Initialize with configuration
    nlohmann::json config = {
        {"mapping_directory", "/path/to/mappings"}
    };
    mapping_handler.init(config);

    // Perform mapping
    std::type_index data_type = std::type_index{typeid(double)};
    int rank = 1;
    nlohmann::json extra_attributes = {{"shot", 12345}};

    auto result = mapping_handler.map(
        "my_mapping",
        "path/to/data",
        data_type,
        rank,
        extra_attributes
    );

    return 0;
}
```

## Documentation Structure

### Getting Started
- [**Using the Library**](using-the-library.md) - Complete guide to integrating and using LibTokaMap in your projects

### Extending LibTokaMap
- [**Creating a New Data Source**](creating-data-source.md) - How to implement custom data sources
- [**Creating Custom Function Libraries**](creating-custom-functions.md) - How to create and register custom mapping functions

## Core Concepts

### Mapping Types

LibTokaMap supports several types of mappings:

- **Value Mapping**: Direct value assignment from JSON configuration
- **Data Source Mapping**: Retrieves data from registered data sources
- **Expression Mapping**: Evaluates mathematical expressions
- **Dimension Mapping**: Handles array dimension information
- **Custom Mapping**: User-defined mapping logic through custom libraries

### Data Sources

Data sources are pluggable components that retrieve data from various backends such as:

- JSON files
- Databases
- Network APIs
- Custom data formats

### Configuration

LibTokaMap uses a hierarchical configuration system based on JSON files:

```
mappings/
├── experiment1/
│   ├── mappings.cfg.json
│   ├── globals.json
│   └── group_name1/
│       └── partition1_0/
│           ├── globals.json
│           └── mappings.json
```

## Building the Library

### Requirements

- C++20 compatible compiler
- CMake 3.15+
- nlohmann/json library
- Pantor/Inja templating engine
- GSL-lite (Guidelines Support Library)
- ExprTk expression parsing library

### Build Instructions

```bash
mkdir build
cd build
cmake .. -DENABLE_TESTING=ON -DENABLE_EXAMPLES=ON
make
```

### CMake Options

- `ENABLE_TESTING`: Build unit tests (default: OFF)
- `ENABLE_EXAMPLES`: Build example applications (default: OFF)

## Examples

The `examples/` directory contains working examples:

- `simple_mapper/`: Basic mapping example with JSON data source

## API Reference

### Main Classes

- **MappingHandler**: Main class for handling mapping operations
- **DataSource**: Base class for implementing custom data sources
- **Mapping**: Base interface for all mapping types
- **TypedDataArray**: Container for typed data arrays
- **RamCache**: Caching system for improved performance

## Contributing

### Code Style
- Follow C++20 best practices
- Use clang-format for code formatting
- Enable all compiler warnings (`-Wall -Werror -Wpedantic`)

### Testing

Run the test suite:

```bash
cd build
make test
```

## License

See the main project LICENSE file for license information.

## Version Information

Current version: 0.1.0