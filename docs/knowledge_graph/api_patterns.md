# LibTokaMap API Patterns and Usage Reference

This document provides comprehensive API patterns and usage examples for LibTokaMap, organized by common use cases and design patterns.

## Core API Patterns

### 1. Basic Library Initialization

```cpp
#include <libtokamap.hpp>

// Method 1: Configuration from JSON object
libtokamap::MappingHandler mapping_handler;
nlohmann::json config = {
    {"mapping_directory", "/path/to/mappings"},
    {"cache_enabled", true},
    {"cache_size", 100}
};
mapping_handler.init(config);

// Method 2: Configuration from file
libtokamap::MappingHandler mapping_handler;
mapping_handler.init("/path/to/config.json");

// Method 3: Minimal initialization
libtokamap::MappingHandler mapping_handler;
nlohmann::json minimal_config = {
    {"mapping_directory", "/path/to/mappings"}
};
mapping_handler.init(minimal_config);
```

### 2. Data Source Registration Patterns

```cpp
// Pattern 1: Built-in data source registration
auto json_source = std::make_unique<JSONDataSource>("/path/to/data");
mapping_handler.register_data_source("JSON", std::move(json_source));

// Pattern 2: Custom data source implementation
class CustomDataSource : public libtokamap::DataSource {
public:
    libtokamap::TypedDataArray get(
        const libtokamap::DataSourceArgs& args,
        const libtokamap::MapArguments& arguments,
        libtokamap::RamCache* ram_cache) override {
        
        // Implementation specific logic
        std::vector<double> data = fetch_data(args);
        return libtokamap::TypedDataArray{data};
    }
};

auto custom_source = std::make_unique<CustomDataSource>();
mapping_handler.register_data_source("CUSTOM", std::move(custom_source));

// Pattern 3: Multiple data source registration
void register_all_sources(libtokamap::MappingHandler& handler) {
    handler.register_data_source("JSON", std::make_unique<JSONDataSource>("/json/data"));
    handler.register_data_source("HDF5", std::make_unique<HDF5DataSource>("/hdf5/data"));
    handler.register_data_source("DB", std::make_unique<DatabaseSource>("connection_string"));
}
```

### 3. Mapping Request Patterns

```cpp
// Pattern 1: Basic mapping request
std::type_index data_type = std::type_index{typeid(double)};
int rank = 1;
nlohmann::json extra_attributes = {{"shot", 12345}, {"time", "2023-01-01"}};

auto result = mapping_handler.map(
    "EXPERIMENT_NAME",
    "path/to/data",
    data_type,
    rank,
    extra_attributes
);

// Pattern 2: Type-safe result extraction
if (result.type_index() == std::type_index{typeid(double)}) {
    auto data_vector = result.to_vector<double>();
    auto data_span = result.span<double>();
    const double* raw_data = result.data<double>();
}

// Pattern 3: Multi-dimensional data handling
std::type_index data_type = std::type_index{typeid(float)};
int rank = 2; // 2D array expected
auto result = mapping_handler.map("EXPERIMENT", "2d/array/path", data_type, rank, attributes);

// Access shape information
const auto& shape = result.shape();
size_t rows = shape[0];
size_t cols = shape[1];
```

## Error Handling Patterns

### 1. Exception Handling Strategy

```cpp
#include <exceptions/exceptions.hpp>

// Pattern 1: Comprehensive error handling
try {
    auto result = mapping_handler.map("EXPERIMENT", "path", type, rank, attrs);
    process_result(result);
} catch (const libtokamap::MappingError& e) {
    std::cerr << "Mapping error: " << e.what() << std::endl;
    // Handle mapping-specific errors
} catch (const libtokamap::DataSourceError& e) {
    std::cerr << "Data source error: " << e.what() << std::endl;
    // Handle data source errors
} catch (const libtokamap::ValidationError& e) {
    std::cerr << "Validation error: " << e.what() << std::endl;
    // Handle configuration validation errors
} catch (const libtokamap::TokaMapError& e) {
    std::cerr << "General error: " << e.what() << std::endl;
    // Handle any other library errors
}

// Pattern 2: Error recovery with fallbacks
libtokamap::TypedDataArray safe_map(
    libtokamap::MappingHandler& handler,
    const std::string& experiment,
    const std::string& path,
    std::type_index type,
    int rank,
    const nlohmann::json& attrs) {
    
    try {
        return handler.map(experiment, path, type, rank, attrs);
    } catch (const libtokamap::MappingError&) {
        // Try alternative mapping
        std::string fallback_path = path + "_fallback";
        try {
            return handler.map(experiment, fallback_path, type, rank, attrs);
        } catch (const libtokamap::MappingError&) {
            // Return default value
            if (type == std::type_index{typeid(double)}) {
                return libtokamap::TypedDataArray{0.0};
            }
            throw; // Re-throw if no fallback possible
        }
    }
}
```

### 2. Validation and Debugging Patterns

```cpp
// Pattern 1: Pre-validation
bool validate_request(const std::string& experiment, const std::string& path) {
    // Check experiment exists
    // Validate path format
    // Check permissions
    return true;
}

// Pattern 2: Debug information extraction
void debug_mapping_result(const libtokamap::TypedDataArray& result) {
    std::cout << "Result type: " << result.type_index().name() << std::endl;
    std::cout << "Size: " << result.size() << std::endl;
    std::cout << "Rank: " << result.rank() << std::endl;
    std::cout << "Shape: [";
    for (size_t i = 0; i < result.shape().size(); ++i) {
        if (i > 0) std::cout << ", ";
        std::cout << result.shape()[i];
    }
    std::cout << "]" << std::endl;
    std::cout << "Data preview: " << result.to_string(5) << std::endl;
}
```

## Advanced Usage Patterns

### 1. Custom Mapping Functions

```cpp
// Pattern 1: External library function definition
extern "C" libtokamap::TypedDataArray custom_interpolation(
    const libtokamap::CustomMappingInputMap& inputs,
    const libtokamap::CustomMappingParams& params) {
    
    // Extract input data
    auto x_data = inputs.at("x_values");
    auto y_data = inputs.at("y_values");
    
    // Extract parameters
    auto method = params.at("method").get<std::string>();
    auto num_points = params.at("num_points").get<int>();
    
    // Perform interpolation
    std::vector<double> result = perform_interpolation(x_data, y_data, method, num_points);
    
    return libtokamap::TypedDataArray{result};
}

// Pattern 2: Dynamic library registration
void register_custom_functions(libtokamap::MappingHandler& handler) {
    std::vector<std::string> library_paths = {
        "/usr/local/lib/libtokamap_extensions.so",
        "./custom_functions.so"
    };
    
    for (const auto& lib_path : library_paths) {
        try {
            auto func = load_library_function(lib_path, "custom_interpolation");
            handler.register_custom_function(func);
        } catch (const std::exception& e) {
            std::cerr << "Failed to load " << lib_path << ": " << e.what() << std::endl;
        }
    }
}
```

### 2. Performance Optimization Patterns

```cpp
// Pattern 1: Cache management
class OptimizedMappingHandler {
private:
    libtokamap::MappingHandler handler;
    std::unordered_map<std::string, libtokamap::TypedDataArray> local_cache;
    
public:
    libtokamap::TypedDataArray cached_map(
        const std::string& experiment,
        const std::string& path,
        std::type_index type,
        int rank,
        const nlohmann::json& attrs) {
        
        // Create cache key
        std::string cache_key = experiment + "|" + path + "|" + 
                               std::to_string(type.hash_code());
        
        // Check local cache first
        auto it = local_cache.find(cache_key);
        if (it != local_cache.end()) {
            return std::move(it->second);
        }
        
        // Fetch and cache
        auto result = handler.map(experiment, path, type, rank, attrs);
        local_cache[cache_key] = std::move(result);
        return local_cache[cache_key];
    }
    
    void clear_cache() {
        local_cache.clear();
        // Also clear internal cache if needed
    }
};

// Pattern 2: Batch processing
class BatchProcessor {
public:
    struct BatchRequest {
        std::string experiment;
        std::string path;
        std::type_index type;
        int rank;
        nlohmann::json attributes;
    };
    
    std::vector<libtokamap::TypedDataArray> process_batch(
        libtokamap::MappingHandler& handler,
        const std::vector<BatchRequest>& requests) {
        
        std::vector<libtokamap::TypedDataArray> results;
        results.reserve(requests.size());
        
        for (const auto& request : requests) {
            try {
                results.emplace_back(handler.map(
                    request.experiment,
                    request.path,
                    request.type,
                    request.rank,
                    request.attributes
                ));
            } catch (const std::exception& e) {
                std::cerr << "Batch item failed: " << e.what() << std::endl;
                // Add empty result or skip
                results.emplace_back();
            }
        }
        
        return results;
    }
};
```

### 3. Configuration Management Patterns

```cpp
// Pattern 1: Environment-aware configuration
nlohmann::json create_environment_config() {
    nlohmann::json config;
    
    // Base configuration
    config["cache_enabled"] = true;
    config["cache_size"] = 1000;
    
    // Environment-specific overrides
    const char* env = std::getenv("TOKAMAP_ENV");
    if (env && std::string{env} == "production") {
        config["mapping_directory"] = "/opt/tokamap/mappings";
        config["cache_size"] = 10000;
        config["logging_level"] = "error";
    } else if (env && std::string{env} == "development") {
        config["mapping_directory"] = "./dev_mappings";
        config["cache_size"] = 100;
        config["logging_level"] = "debug";
    } else {
        config["mapping_directory"] = "./mappings";
        config["logging_level"] = "info";
    }
    
    return config;
}

// Pattern 2: Configuration validation
bool validate_configuration(const nlohmann::json& config) {
    // Required fields
    if (!config.contains("mapping_directory")) {
        std::cerr << "Missing required field: mapping_directory" << std::endl;
        return false;
    }
    
    // Path validation
    std::filesystem::path mapping_dir = config["mapping_directory"];
    if (!std::filesystem::exists(mapping_dir)) {
        std::cerr << "Mapping directory does not exist: " << mapping_dir << std::endl;
        return false;
    }
    
    // Optional field validation
    if (config.contains("cache_size")) {
        auto cache_size = config["cache_size"].get<int>();
        if (cache_size < 0) {
            std::cerr << "Invalid cache size: " << cache_size << std::endl;
            return false;
        }
    }
    
    return true;
}
```

## Integration Patterns

### 1. RAII and Resource Management

```cpp
// Pattern 1: RAII wrapper for MappingHandler
class ScopedMappingHandler {
private:
    libtokamap::MappingHandler handler;
    bool initialized = false;
    
public:
    explicit ScopedMappingHandler(const nlohmann::json& config) {
        try {
            handler.init(config);
            initialized = true;
        } catch (const std::exception& e) {
            std::cerr << "Failed to initialize: " << e.what() << std::endl;
            throw;
        }
    }
    
    ~ScopedMappingHandler() {
        if (initialized) {
            handler.reset();
        }
    }
    
    // Non-copyable, movable
    ScopedMappingHandler(const ScopedMappingHandler&) = delete;
    ScopedMappingHandler& operator=(const ScopedMappingHandler&) = delete;
    ScopedMappingHandler(ScopedMappingHandler&&) = default;
    ScopedMappingHandler& operator=(ScopedMappingHandler&&) = default;
    
    libtokamap::TypedDataArray map(
        const std::string& experiment,
        const std::string& path,
        std::type_index type,
        int rank,
        const nlohmann::json& attrs) {
        if (!initialized) {
            throw std::runtime_error("Handler not initialized");
        }
        return handler.map(experiment, path, type, rank, attrs);
    }
    
    void register_data_source(const std::string& name, 
                             std::unique_ptr<libtokamap::DataSource> source) {
        if (!initialized) {
            throw std::runtime_error("Handler not initialized");
        }
        handler.register_data_source(name, std::move(source));
    }
};

// Usage
{
    ScopedMappingHandler handler{config};
    auto result = handler.map("EXPERIMENT", "path", type, rank, attrs);
    // Automatic cleanup on scope exit
}
```

### 2. Factory Patterns

```cpp
// Pattern 1: Data source factory
class DataSourceFactory {
public:
    static std::unique_ptr<libtokamap::DataSource> create(
        const std::string& type,
        const nlohmann::json& config) {
        
        if (type == "JSON") {
            std::string root_path = config["root_path"];
            return std::make_unique<JSONDataSource>(root_path);
        } else if (type == "HDF5") {
            std::string file_path = config["file_path"];
            return std::make_unique<HDF5DataSource>(file_path);
        } else if (type == "DATABASE") {
            std::string connection = config["connection_string"];
            return std::make_unique<DatabaseSource>(connection);
        }
        
        throw std::invalid_argument("Unknown data source type: " + type);
    }
    
    static void register_all(libtokamap::MappingHandler& handler,
                           const nlohmann::json& sources_config) {
        for (const auto& [name, config] : sources_config.items()) {
            std::string type = config["type"];
            auto source = create(type, config);
            handler.register_data_source(name, std::move(source));
        }
    }
};

// Pattern 2: Configuration factory
class ConfigurationFactory {
public:
    static nlohmann::json from_file(const std::string& path) {
        std::ifstream file{path};
        if (!file.is_open()) {
            throw std::runtime_error("Cannot open config file: " + path);
        }
        
        nlohmann::json config;
        file >> config;
        return config;
    }
    
    static nlohmann::json from_environment() {
        nlohmann::json config;
        
        if (const char* mapping_dir = std::getenv("TOKAMAP_MAPPING_DIR")) {
            config["mapping_directory"] = mapping_dir;
        }
        
        if (const char* cache_size = std::getenv("TOKAMAP_CACHE_SIZE")) {
            config["cache_size"] = std::stoi(cache_size);
        }
        
        if (const char* cache_enabled = std::getenv("TOKAMAP_CACHE_ENABLED")) {
            config["cache_enabled"] = std::string{cache_enabled} == "true";
        }
        
        return config;
    }
    
    static nlohmann::json merge_configs(const std::vector<nlohmann::json>& configs) {
        nlohmann::json merged;
        for (const auto& config : configs) {
            merged.merge_patch(config);
        }
        return merged;
    }
};
```

### 3. Observer/Callback Patterns

```cpp
// Pattern 1: Progress monitoring
class MappingProgressMonitor {
public:
    using ProgressCallback = std::function<void(const std::string&, double)>;
    using ErrorCallback = std::function<void(const std::string&, const std::exception&)>;
    
    void set_progress_callback(ProgressCallback callback) {
        progress_callback = std::move(callback);
    }
    
    void set_error_callback(ErrorCallback callback) {
        error_callback = std::move(callback);
    }
    
    libtokamap::TypedDataArray monitored_map(
        libtokamap::MappingHandler& handler,
        const std::string& experiment,
        const std::string& path,
        std::type_index type,
        int rank,
        const nlohmann::json& attrs) {
        
        std::string operation = experiment + "/" + path;
        
        if (progress_callback) {
            progress_callback(operation, 0.0);
        }
        
        try {
            auto result = handler.map(experiment, path, type, rank, attrs);
            
            if (progress_callback) {
                progress_callback(operation, 1.0);
            }
            
            return result;
        } catch (const std::exception& e) {
            if (error_callback) {
                error_callback(operation, e);
            }
            throw;
        }
    }
    
private:
    ProgressCallback progress_callback;
    ErrorCallback error_callback;
};

// Usage
MappingProgressMonitor monitor;
monitor.set_progress_callback([](const std::string& op, double progress) {
    std::cout << op << ": " << (progress * 100) << "%" << std::endl;
});

monitor.set_error_callback([](const std::string& op, const std::exception& e) {
    std::cerr << "Error in " << op << ": " << e.what() << std::endl;
});

auto result = monitor.monitored_map(handler, "EXP", "path", type, rank, attrs);
```

## Testing Patterns

### 1. Mock Data Sources

```cpp
// Pattern 1: Mock data source for testing
class MockDataSource : public libtokamap::DataSource {
private:
    std::unordered_map<std::string, libtokamap::TypedDataArray> mock_data;
    
public:
    void add_mock_data(const std::string& key, libtokamap::TypedDataArray data) {
        mock_data[key] = std::move(data);
    }
    
    libtokamap::TypedDataArray get(
        const libtokamap::DataSourceArgs& args,
        const libtokamap::MapArguments& arguments,
        libtokamap::RamCache* ram_cache) override {
        
        std::string key = args.at("path").get<std::string>();
        auto it = mock_data.find(key);
        if (it != mock_data.end()) {
            // Return copy of mock data
            return std::move(it->second);
        }
        
        throw libtokamap::DataSourceError("Mock data not found: " + key);
    }
};

// Test usage
void test_mapping_functionality() {
    libtokamap::MappingHandler handler;
    
    // Setup mock data source
    auto mock_source = std::make_unique<MockDataSource>();
    std::vector<double> test_data = {1.0, 2.0, 3.0, 4.0, 5.0};
    mock_source->add_mock_data("test/path", libtokamap::TypedDataArray{test_data});
    
    handler.register_data_source("MOCK", std::move(mock_source));
    
    // Configure handler with test mappings
    nlohmann::json config = {{"mapping_directory", "./test_mappings"}};
    handler.init(config);
    
    // Test mapping
    auto result = handler.map("TEST_EXPERIMENT", "test/path", 
                             std::type_index{typeid(double)}, 1, {});
    
    assert(result.size() == 5);
    assert(result.to_vector<double>() == test_data);
}
```

This comprehensive API patterns reference provides developers with practical examples and best practices for using LibTokaMap effectively in various scenarios, from basic usage to advanced integration patterns.