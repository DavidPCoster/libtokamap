# Creating Custom Function Libraries

This guide explains how to create and use custom function libraries in LibTokaMap. Custom functions allow you to extend the mapping system with user-defined logic that can perform complex data transformations, calculations, or any custom processing.

## Overview

Custom functions in LibTokaMap are implemented as C++ functions or classes that are loaded from shared libraries (.so files on Linux, .dll on Windows). These functions can access multiple input data arrays and return transformed data based on custom parameters.

## Basic Custom Function Implementation

### Step 1: Understand the Function Signature

All custom functions must conform to this signature:

```cpp
libtokamap::TypedDataArray my_function(
    libtokamap::CustomMappingInputs& inputs,
    const libtokamap::CustomMappingParams& params
);
```

Where:
- `CustomMappingInputs` is `std::unordered_map<std::string, libtokamap::TypedDataArray>`
- `CustomMappingParams` is `nlohmann::json`
- Return type is `libtokamap::TypedDataArray`

### Step 2: Create Your Function Library

Create a source file (e.g., `my_custom_functions.cpp`):

```cpp
#include <libtokamap.hpp>
#include <nlohmann/json.hpp>
#include <vector>
#include <cmath>
#include <algorithm>

extern "C" {

// Simple mathematical transformation
libtokamap::TypedDataArray polynomial_transform(
    libtokamap::CustomMappingInputs& inputs,
    const libtokamap::CustomMappingParams& params)
{
    // Get input data
    if (inputs.find("data") == inputs.end()) {
        throw libtokamap::MappingError("Missing 'data' input");
    }
    
    auto& input_data = inputs["data"];
    if (!input_data.holds_alternative<std::vector<double>>()) {
        throw libtokamap::MappingError("Input 'data' must be vector<double>");
    }
    
    auto data = std::get<std::vector<double>>(input_data);
    
    // Get parameters
    double a = params.value("a", 1.0);
    double b = params.value("b", 0.0);
    double c = params.value("c", 0.0);
    
    // Apply polynomial transformation: y = ax² + bx + c
    std::vector<double> result;
    result.reserve(data.size());
    
    for (double x : data) {
        double y = a * x * x + b * x + c;
        result.push_back(y);
    }
    
    return libtokamap::TypedDataArray{result};
}

// Statistical analysis function
libtokamap::TypedDataArray statistical_analysis(
    libtokamap::CustomMappingInputs& inputs,
    const libtokamap::CustomMappingParams& params)
{
    if (inputs.find("data") == inputs.end()) {
        throw libtokamap::MappingError("Missing 'data' input");
    }
    
    auto& input_data = inputs["data"];
    if (!input_data.holds_alternative<std::vector<double>>()) {
        throw libtokamap::MappingError("Input 'data' must be vector<double>");
    }
    
    auto data = std::get<std::vector<double>>(input_data);
    std::string analysis_type = params.value("type", "mean");
    
    std::vector<double> result;
    
    if (analysis_type == "mean") {
        double sum = 0.0;
        for (double val : data) sum += val;
        result.push_back(sum / data.size());
        
    } else if (analysis_type == "std") {
        // Calculate standard deviation
        double mean = 0.0;
        for (double val : data) mean += val;
        mean /= data.size();
        
        double variance = 0.0;
        for (double val : data) {
            double diff = val - mean;
            variance += diff * diff;
        }
        variance /= data.size();
        
        result.push_back(std::sqrt(variance));
        
    } else if (analysis_type == "minmax") {
        auto minmax = std::minmax_element(data.begin(), data.end());
        result.push_back(*minmax.first);
        result.push_back(*minmax.second);
        
    } else {
        throw libtokamap::MappingError("Unknown analysis type: " + analysis_type);
    }
    
    return libtokamap::TypedDataArray{result};
}

// Multi-input function example
libtokamap::TypedDataArray vector_operations(
    libtokamap::CustomMappingInputs& inputs,
    const libtokamap::CustomMappingParams& params)
{
    // Validate required inputs
    if (inputs.find("vector1") == inputs.end() || inputs.find("vector2") == inputs.end()) {
        throw libtokamap::MappingError("Missing required inputs: vector1, vector2");
    }
    
    auto& v1_data = inputs["vector1"];
    auto& v2_data = inputs["vector2"];
    
    if (!v1_data.holds_alternative<std::vector<double>>() ||
        !v2_data.holds_alternative<std::vector<double>>()) {
        throw libtokamap::MappingError("Both inputs must be vector<double>");
    }
    
    auto v1 = std::get<std::vector<double>>(v1_data);
    auto v2 = std::get<std::vector<double>>(v2_data);
    
    if (v1.size() != v2.size()) {
        throw libtokamap::MappingError("Input vectors must have the same size");
    }
    
    std::string operation = params.value("operation", "add");
    std::vector<double> result;
    result.reserve(v1.size());
    
    if (operation == "add") {
        for (size_t i = 0; i < v1.size(); ++i) {
            result.push_back(v1[i] + v2[i]);
        }
    } else if (operation == "multiply") {
        for (size_t i = 0; i < v1.size(); ++i) {
            result.push_back(v1[i] * v2[i]);
        }
    } else if (operation == "dot_product") {
        double dot = 0.0;
        for (size_t i = 0; i < v1.size(); ++i) {
            dot += v1[i] * v2[i];
        }
        result.push_back(dot);
    } else {
        throw libtokamap::MappingError("Unknown operation: " + operation);
    }
    
    return libtokamap::TypedDataArray{result};
}

// Library entry point - required for dynamic loading
libtokamap::LibraryEntryInterface* get_library_entry() {
    static libtokamap::LibraryEntryInterface entry;
    
    // Register all functions in this library
    entry.functions.emplace_back(
        "math_functions", "polynomial_transform",
        std::make_unique<libtokamap::LibraryFunctionPointer>(polynomial_transform)
    );
    
    entry.functions.emplace_back(
        "math_functions", "statistical_analysis",
        std::make_unique<libtokamap::LibraryFunctionPointer>(statistical_analysis)
    );
    
    entry.functions.emplace_back(
        "math_functions", "vector_operations",
        std::make_unique<libtokamap::LibraryFunctionPointer>(vector_operations)
    );
    
    return &entry;
}

} // extern "C"
```

### Step 3: Build the Shared Library

Create a CMakeLists.txt for your custom functions:

```cmake
cmake_minimum_required(VERSION 3.15)
project(MyCustomFunctions)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find LibTokaMap
find_package(libtokamap REQUIRED)

# Create shared library
add_library(my_custom_functions SHARED
    my_custom_functions.cpp
)

target_link_libraries(my_custom_functions
    libtokamap::libtokamap
)

# Set library properties
set_target_properties(my_custom_functions PROPERTIES
    PREFIX ""  # Remove 'lib' prefix
    POSITION_INDEPENDENT_CODE ON
)
```

Build the library:

```bash
mkdir build
cd build
cmake ..
make
```

This creates `my_custom_functions.so` (or `.dll` on Windows).

## Advanced Custom Function Features

### Handling Different Data Types

Your functions can handle multiple data types:

```cpp
libtokamap::TypedDataArray type_aware_function(
    libtokamap::CustomMappingInputs& inputs,
    const libtokamap::CustomMappingParams& params)
{
    auto& input_data = inputs["data"];
    
    if (input_data.holds_alternative<std::vector<double>>()) {
        auto data = std::get<std::vector<double>>(input_data);
        // Process double data
        std::vector<double> result;
        for (double val : data) {
            result.push_back(val * 2.0);
        }
        return libtokamap::TypedDataArray{result};
        
    } else if (input_data.holds_alternative<std::vector<int>>()) {
        auto data = std::get<std::vector<int>>(input_data);
        // Process int data
        std::vector<int> result;
        for (int val : data) {
            result.push_back(val * 2);
        }
        return libtokamap::TypedDataArray{result};
        
    } else if (input_data.holds_alternative<std::vector<std::string>>()) {
        auto data = std::get<std::vector<std::string>>(input_data);
        // Process string data
        std::vector<std::string> result;
        for (const auto& str : data) {
            result.push_back(str + str);  // Duplicate strings
        }
        return libtokamap::TypedDataArray{result};
        
    } else {
        throw libtokamap::MappingError("Unsupported data type");
    }
}
```

### Complex Parameter Handling

Use JSON parameters for complex configurations:

```cpp
libtokamap::TypedDataArray filtering_function(
    libtokamap::CustomMappingInputs& inputs,
    const libtokamap::CustomMappingParams& params)
{
    auto data = std::get<std::vector<double>>(inputs["data"]);
    
    // Complex parameter structure
    auto filter_config = params["filter"];
    std::string filter_type = filter_config["type"];
    
    if (filter_type == "lowpass") {
        double cutoff = filter_config["cutoff"];
        int order = filter_config.value("order", 4);
        // Implement low-pass filter
        return apply_lowpass_filter(data, cutoff, order);
        
    } else if (filter_type == "bandpass") {
        double low_freq = filter_config["low_frequency"];
        double high_freq = filter_config["high_frequency"];
        // Implement band-pass filter
        return apply_bandpass_filter(data, low_freq, high_freq);
        
    } else if (filter_type == "threshold") {
        double threshold = filter_config["threshold"];
        std::string mode = filter_config.value("mode", "above");
        
        std::vector<double> result;
        for (double val : data) {
            if ((mode == "above" && val > threshold) ||
                (mode == "below" && val < threshold)) {
                result.push_back(val);
            }
        }
        return libtokamap::TypedDataArray{result};
    }
    
    throw libtokamap::MappingError("Unknown filter type: " + filter_type);
}
```

### Error Handling and Validation

Implement comprehensive error handling:

```cpp
libtokamap::TypedDataArray robust_function(
    libtokamap::CustomMappingInputs& inputs,
    const libtokamap::CustomMappingParams& params)
{
    try {
        // Validate required inputs
        std::vector<std::string> required_inputs = {"input1", "input2"};
        for (const auto& input_name : required_inputs) {
            if (inputs.find(input_name) == inputs.end()) {
                throw libtokamap::MappingError("Missing required input: " + input_name);
            }
        }
        
        // Validate parameter structure
        if (!params.contains("config")) {
            throw libtokamap::MappingError("Missing 'config' parameter");
        }
        
        auto config = params["config"];
        if (!config.is_object()) {
            throw libtokamap::MappingError("'config' must be an object");
        }
        
        // Validate data types and sizes
        auto& data1 = inputs["input1"];
        auto& data2 = inputs["input2"];
        
        if (!data1.holds_alternative<std::vector<double>>() ||
            !data2.holds_alternative<std::vector<double>>()) {
            throw libtokamap::MappingError("Inputs must be vector<double>");
        }
        
        auto vec1 = std::get<std::vector<double>>(data1);
        auto vec2 = std::get<std::vector<double>>(data2);
        
        if (vec1.empty() || vec2.empty()) {
            throw libtokamap::MappingError("Input vectors cannot be empty");
        }
        
        if (vec1.size() != vec2.size()) {
            throw libtokamap::MappingError(
                "Input vectors must have same size: " + 
                std::to_string(vec1.size()) + " vs " + std::to_string(vec2.size())
            );
        }
        
        // Perform computation
        std::vector<double> result;
        // ... computation logic ...
        
        return libtokamap::TypedDataArray{result};
        
    } catch (const libtokamap::MappingError&) {
        // Re-throw LibTokaMap errors
        throw;
    } catch (const nlohmann::json::exception& e) {
        // Handle JSON parsing errors
        throw libtokamap::MappingError("JSON error: " + std::string(e.what()));
    } catch (const std::exception& e) {
        // Handle other standard exceptions
        throw libtokamap::MappingError("Function error: " + std::string(e.what()));
    }
}
```

## Loading and Using Custom Functions

### Method 1: Dynamic Loading

Load your library at runtime:

```cpp
#include <libtokamap.hpp>

int main() {
    try {
        // Load custom functions from shared library
        auto functions = libtokamap::load_custom_functions("./my_custom_functions.so");
        
        // Initialize mapping handler with custom functions
        libtokamap::MappingHandler handler;
        
        // Register functions
        for (auto& func : functions) {
            handler.register_custom_function(std::move(func));
        }
        
        // Continue with normal usage...
        
    } catch (const libtokamap::TokaMapError& e) {
        std::cerr << "Error loading custom functions: " << e.what() << std::endl;
        return 1;
    }
}
```

### Method 2: Static Registration

For static linking, register functions directly:

```cpp
// In your main application
libtokamap::MappingHandler handler;

// Register individual functions
handler.register_custom_function(libtokamap::LibraryFunction{
    "math_functions", "polynomial_transform",
    std::make_unique<libtokamap::LibraryFunctionPointer>(polynomial_transform)
});
```

## Configuration in Mappings

Use your custom functions in mapping JSON files:

```json
{
    "processed_temperature": {
        "map_type": "CUSTOM",
        "custom_type": "polynomial_transform",
        "library": "math_functions",
        "inputs": {
            "data": "raw_temperature_data"
        },
        "params": {
            "a": 0.001,
            "b": 1.0,
            "c": -273.15
        }
    },
    
    "temperature_stats": {
        "map_type": "CUSTOM",
        "custom_type": "statistical_analysis",
        "library": "math_functions",
        "inputs": {
            "data": "processed_temperature"
        },
        "params": {
            "type": "minmax"
        }
    },
    
    "combined_vectors": {
        "map_type": "CUSTOM",
        "custom_type": "vector_operations",
        "library": "math_functions",
        "inputs": {
            "vector1": "magnetic_field_x",
            "vector2": "magnetic_field_y"
        },
        "params": {
            "operation": "dot_product"
        }
    },
    
    "filtered_data": {
        "map_type": "CUSTOM",
        "custom_type": "filtering_function",
        "library": "signal_processing",
        "inputs": {
            "data": "noisy_signal"
        },
        "params": {
            "filter": {
                "type": "bandpass",
                "low_frequency": 10.0,
                "high_frequency": 100.0
            }
        }
    }
}
```

## Real-World Examples

### Signal Processing Library

```cpp
extern "C" {

libtokamap::TypedDataArray fft_transform(
    libtokamap::CustomMappingInputs& inputs,
    const libtokamap::CustomMappingParams& params)
{
    auto data = std::get<std::vector<double>>(inputs["signal"]);
    bool return_magnitude = params.value("magnitude_only", true);
    
    // Implement FFT (using FFTW or similar)
    auto fft_result = compute_fft(data);
    
    if (return_magnitude) {
        std::vector<double> magnitudes;
        for (const auto& complex_val : fft_result) {
            magnitudes.push_back(std::abs(complex_val));
        }
        return libtokamap::TypedDataArray{magnitudes};
    } else {
        // Return interleaved real/imaginary parts
        std::vector<double> interleaved;
        for (const auto& complex_val : fft_result) {
            interleaved.push_back(complex_val.real());
            interleaved.push_back(complex_val.imag());
        }
        return libtokamap::TypedDataArray{interleaved};
    }
}

libtokamap::TypedDataArray moving_average(
    libtokamap::CustomMappingInputs& inputs,
    const libtokamap::CustomMappingParams& params)
{
    auto data = std::get<std::vector<double>>(inputs["data"]);
    int window_size = params.value("window_size", 5);
    
    if (window_size <= 0 || window_size > static_cast<int>(data.size())) {
        throw libtokamap::MappingError("Invalid window size");
    }
    
    std::vector<double> result;
    result.reserve(data.size() - window_size + 1);
    
    for (size_t i = 0; i <= data.size() - window_size; ++i) {
        double sum = 0.0;
        for (int j = 0; j < window_size; ++j) {
            sum += data[i + j];
        }
        result.push_back(sum / window_size);
    }
    
    return libtokamap::TypedDataArray{result};
}

} // extern "C"
```

### Machine Learning Library

```cpp
extern "C" {

libtokamap::TypedDataArray normalize_data(
    libtokamap::CustomMappingInputs& inputs,
    const libtokamap::CustomMappingParams& params)
{
    auto data = std::get<std::vector<double>>(inputs["data"]);
    std::string method = params.value("method", "minmax");
    
    if (method == "minmax") {
        auto [min_val, max_val] = std::minmax_element(data.begin(), data.end());
        double range = *max_val - *min_val;
        
        if (range == 0.0) {
            return libtokamap::TypedDataArray{std::vector<double>(data.size(), 0.0)};
        }
        
        std::vector<double> normalized;
        normalized.reserve(data.size());
        
        for (double val : data) {
            normalized.push_back((val - *min_val) / range);
        }
        
        return libtokamap::TypedDataArray{normalized};
        
    } else if (method == "zscore") {
        // Calculate mean
        double mean = 0.0;
        for (double val : data) mean += val;
        mean /= data.size();
        
        // Calculate standard deviation
        double variance = 0.0;
        for (double val : data) {
            double diff = val - mean;
            variance += diff * diff;
        }
        double std_dev = std::sqrt(variance / data.size());
        
        if (std_dev == 0.0) {
            return libtokamap::TypedDataArray{std::vector<double>(data.size(), 0.0)};
        }
        
        std::vector<double> normalized;
        normalized.reserve(data.size());
        
        for (double val : data) {
            normalized.push_back((val - mean) / std_dev);
        }
        
        return libtokamap::TypedDataArray{normalized};
    }
    
    throw libtokamap::MappingError("Unknown normalization method: " + method);
}

} // extern "C"
```

## Best Practices

### Performance Optimization

1. **Reserve vector capacity**: Use `std::vector::reserve()` when possible
2. **Avoid unnecessary copying**: Use references and move semantics
3. **Consider parallelization**: Use OpenMP or similar for CPU-intensive tasks
4. **Memory management**: Be careful with large data sets

```cpp
libtokamap::TypedDataArray optimized_function(
    libtokamap::CustomMappingInputs& inputs,
    const libtokamap::CustomMappingParams& params)
{
    const auto& data = std::get<std::vector<double>>(inputs["data"]);
    
    std::vector<double> result;
    result.reserve(data.size());  // Pre-allocate memory
    
    // Use parallel processing for large datasets
    #pragma omp parallel for
    for (size_t i = 0; i < data.size(); ++i) {
        // Expensive computation
        double processed = expensive_computation(data[i]);
        
        #pragma omp critical
        result.push_back(processed);
    }
    
    return libtokamap::TypedDataArray{std::move(result)};
}
```

### Error Handling

1. **Validate inputs early**: Check all inputs and parameters at the start
2. **Provide meaningful messages**: Include context in error messages
3. **Handle edge cases**: Consider empty inputs, invalid parameters
4. **Use appropriate exception types**: Use LibTokaMap exception classes

### Code Organization

1. **Separate concerns**: Create focused, single-purpose functions
2. **Use helper functions**: Break complex logic into smaller pieces
3. **Document parameters**: Clearly document expected parameters and formats
4. **Version your libraries**: Consider versioning for backward compatibility

### Testing

Create unit tests for your custom functions:

```cpp
#include <gtest/gtest.h>
#include "my_custom_functions.hpp"

TEST(CustomFunctionsTest, PolynomialTransform) {
    libtokamap::CustomMappingInputs inputs;
    inputs["data"] = libtokamap::TypedDataArray{std::vector<double>{1.0, 2.0, 3.0}};
    
    nlohmann::json params = {
        {"a", 1.0},
        {"b", 0.0},
        {"c", 0.0}
    };
    
    auto result = polynomial_transform(inputs, params);
    
    ASSERT_TRUE(result.holds_alternative<std::vector<double>>());
    auto data = std::get<std::vector<double>>(result);
    
    EXPECT_DOUBLE_EQ(data[0], 1.0);  // 1^2 = 1
    EXPECT_DOUBLE_EQ(data[1], 4.0);  // 2^2 = 4
    EXPECT_DOUBLE_EQ(data[2], 9.0);  // 3^2 = 9
}

TEST(CustomFunctionsTest, StatisticalAnalysis) {
    libtokamap::CustomMappingInputs inputs;
    inputs["data"] = libtokamap::TypedDataArray{std::vector<double>{1.0, 2.0, 3.0, 4.0, 5.0}};
    
    nlohmann::json params = {{"type", "mean"}};
    
    auto result = statistical_analysis(inputs, params);
    
    ASSERT_TRUE(result.holds_alternative<std::vector<double>>());
    auto data = std::get<std::vector<double>>(result);
    
    EXPECT_DOUBLE_EQ(data[0], 3.0);  // Mean of 1,2,3,4,5 = 3
}
```

## Troubleshooting

### Common Issues

1. **Function not found**: Check library loading and registration
2. **Type mismatches**: Verify input data types match expectations
3. **Parameter errors**: Validate JSON parameter structure
4. **Memory errors**: Check for buffer overruns and memory leaks
5. **Linking errors**: Ensure all dependencies are properly linked

### Debugging Tips

1. **Add logging**: Use appropriate logging framework for diagnostics
2. **Test incrementally**: Test functions in isolation first
3. **Check memory usage**: Use tools like Valgrind for memory analysis
4. **Validate JSON**: Use JSON schema validation for parameters
5. **Use sanitizers**: Enable AddressSanitizer and other runtime checks

### Build Issues

1. **Missing headers**: Ensure LibTokaMap headers are in include path
2. **Linking failures**: Check that all required libraries are linked
3. **ABI compatibility**: Ensure consistent compiler flags and standard library versions
4. **Symbol visibility**: Use `extern "C"` for entry points

This comprehensive guide provides everything needed to create powerful custom function libraries for LibTokaMap. The examples demonstrate various patterns and best practices for different use cases.