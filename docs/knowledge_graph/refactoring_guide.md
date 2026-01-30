# LibTokaMap Refactoring Guide

This guide identifies key refactoring opportunities in the LibTokaMap codebase and provides practical migration paths to improve maintainability, performance, and modern C++ usage.

## Overview

Based on the analysis of the codebase and TODO items, this guide covers:
- Immediate modernization opportunities  
- Architectural improvements
- Performance optimizations
- API evolution strategies
- Migration timelines and compatibility considerations

## High Priority Refactoring Opportunities

### 1. Type System Modernization

#### Current State
- Mixed usage of `std::type_index` and `DataType` enum
- Manual type mapping with switch statements
- Runtime type checking with potential errors

#### Proposed Changes
```cpp
// Current approach
std::type_index data_type = std::type_index{typeid(double)};
auto result = handler.map("EXP", "path", data_type, rank, attrs);

// Proposed approach with strong typing
template<typename T>
TypedDataArray<T> map(const std::string& experiment, 
                      const std::string& path,
                      int rank,
                      const nlohmann::json& attrs) {
    constexpr DataType dt = type_to_enum_v<T>;
    return map_impl(experiment, path, dt, rank, attrs);
}

// Usage becomes type-safe at compile time
auto result = handler.map<double>("EXP", "path", rank, attrs);
```

#### Migration Strategy
1. **Phase 1**: Add templated overloads alongside existing API
2. **Phase 2**: Deprecate `std::type_index` versions
3. **Phase 3**: Remove deprecated API in next major version

### 2. Error Handling with std::expected

#### Current State
- Exception-based error handling
- Limited error context information
- Performance overhead in hot paths

#### Proposed Changes
```cpp
// Current approach
try {
    auto result = handler.map("EXP", "path", type, rank, attrs);
    process(result);
} catch (const TokaMapError& e) {
    handle_error(e);
}

// Proposed approach with std::expected (C++23)
auto result = handler.try_map("EXP", "path", type, rank, attrs);
if (result) {
    process(result.value());
} else {
    handle_error(result.error());
}

// Error type with rich context
struct MappingError {
    ErrorCode code;
    std::string message;
    std::string path;
    std::source_location location;
    std::optional<std::exception_ptr> cause;
};
```

#### Migration Strategy
1. Add `try_*` variants returning `std::expected`
2. Maintain exception-based API for backward compatibility
3. Gradually migrate internal error handling
4. Eventually make exception API opt-in

### 3. Immutable Configuration Objects

#### Current State
- Mutable configuration state
- Potential race conditions
- Complex state management

#### Proposed Changes
```cpp
// Current mutable approach
class MappingHandler {
    nlohmann::json m_config;
    bool m_init = false;
    // ... mutable state
};

// Proposed immutable approach
class ImmutableConfig {
public:
    static ConfigBuilder builder();
    
    // All getters, no setters
    const std::filesystem::path& mapping_directory() const;
    bool cache_enabled() const;
    size_t cache_size() const;
    
private:
    // Immutable after construction
    const std::filesystem::path m_mapping_directory;
    const bool m_cache_enabled;
    const size_t m_cache_size;
    // ...
};

class MappingHandler {
public:
    explicit MappingHandler(ImmutableConfig config);
    // No init() method needed
};
```

#### Benefits
- Thread safety by design
- Clearer ownership semantics
- Easier testing and reasoning
- Prevents configuration drift

### 4. Modern String Handling

#### Current State
- String concatenation for formatting
- Manual string building
- Inconsistent string_view usage

#### Proposed Changes
```cpp
// Current approach
std::string create_cache_key(const std::string& experiment,
                           const std::string& path,
                           std::type_index type) {
    return experiment + "|" + path + "|" + std::to_string(type.hash_code());
}

// Proposed with std::format (C++20)
std::string create_cache_key(std::string_view experiment,
                           std::string_view path,
                           DataType type) {
    return std::format("{}|{}|{}", experiment, path, static_cast<int>(type));
}

// For performance-critical paths, use format_to
void append_cache_key(std::string& buffer,
                     std::string_view experiment,
                     std::string_view path,
                     DataType type) {
    std::format_to(std::back_inserter(buffer), 
                   "{}|{}|{}", experiment, path, static_cast<int>(type));
}
```

### 5. Concepts and Template Constraints

#### Current State
- Unconstrained templates
- Runtime type checking
- Generic error messages

#### Proposed Changes
```cpp
// Define concepts for better constraints
template<typename T>
concept SupportedDataType = requires {
    requires std::is_arithmetic_v<T> || std::is_same_v<T, std::string>;
};

template<typename DataSource>
concept ValidDataSource = requires(DataSource ds, 
                                 const DataSourceArgs& args,
                                 const MapArguments& map_args,
                                 RamCache* cache) {
    { ds.get(args, map_args, cache) } -> std::same_as<TypedDataArray>;
};

// Usage with concepts
template<SupportedDataType T>
TypedDataArray create_typed_array(const std::vector<T>& data) {
    return TypedDataArray{data};
}

template<ValidDataSource DS>
void register_data_source(const std::string& name, 
                         std::unique_ptr<DS> source) {
    // Implementation with compile-time guarantees
}
```

## Medium Priority Architectural Improvements

### 1. Functional Mapping Composition

#### Current State
- Imperative mapping pipeline
- Tight coupling between steps
- Limited reusability

#### Proposed Changes
```cpp
// Functional pipeline approach
class MappingPipeline {
public:
    template<typename T>
    using Transform = std::function<TypedDataArray(const TypedDataArray&)>;
    
    MappingPipeline& then(Transform<void> transform) {
        transforms_.push_back(std::move(transform));
        return *this;
    }
    
    TypedDataArray execute(TypedDataArray input) const {
        return std::reduce(transforms_.begin(), transforms_.end(),
                          std::move(input),
                          [](TypedDataArray data, const auto& transform) {
                              return transform(data);
                          });
    }
    
private:
    std::vector<Transform<void>> transforms_;
};

// Usage
auto pipeline = MappingPipeline{}
    .then(apply_scale_offset(2.0, 1.0))
    .then(apply_slice("[0:10:2]"))
    .then(apply_validation());

auto result = pipeline.execute(raw_data);
```

### 2. Async Data Loading with Coroutines

#### Current State
- Synchronous data loading
- Blocking I/O operations
- Limited parallelization

#### Proposed Changes
```cpp
// Coroutine-based async loading
#include <coroutine>

class AsyncMappingHandler {
public:
    struct AsyncResult {
        struct promise_type {
            AsyncResult get_return_object() {
                return AsyncResult{std::coroutine_handle<promise_type>::from_promise(*this)};
            }
            std::suspend_never initial_suspend() { return {}; }
            std::suspend_never final_suspend() noexcept { return {}; }
            void return_value(TypedDataArray result) { 
                value = std::move(result); 
            }
            void unhandled_exception() { 
                exception = std::current_exception(); 
            }
            
            TypedDataArray value;
            std::exception_ptr exception;
        };
        
        std::coroutine_handle<promise_type> coro;
        
        TypedDataArray get() {
            if (!coro.done()) {
                // Wait for completion or implement proper awaiting
            }
            if (coro.promise().exception) {
                std::rethrow_exception(coro.promise().exception);
            }
            return std::move(coro.promise().value);
        }
    };
    
    AsyncResult map_async(const std::string& experiment,
                         const std::string& path,
                         DataType type,
                         int rank,
                         const nlohmann::json& attrs) {
        // Coroutine implementation
        co_return co_await load_data_async(experiment, path, type, rank, attrs);
    }
};
```

### 3. Plugin Hot-Reloading

#### Current State
- Static plugin registration
- Requires restart for plugin changes
- Limited development flexibility

#### Proposed Changes
```cpp
class HotReloadablePluginManager {
public:
    struct PluginInfo {
        std::filesystem::path library_path;
        std::filesystem::file_time_type last_modified;
        void* handle = nullptr;
        std::vector<std::string> exported_functions;
    };
    
    void enable_hot_reload(std::chrono::milliseconds check_interval = 1000ms) {
        hot_reload_enabled_ = true;
        reload_thread_ = std::jthread([this, check_interval](std::stop_token token) {
            while (!token.stop_requested()) {
                check_and_reload_plugins();
                std::this_thread::sleep_for(check_interval);
            }
        });
    }
    
    void register_plugin_directory(const std::filesystem::path& dir) {
        watch_directories_.push_back(dir);
    }
    
private:
    void check_and_reload_plugins() {
        for (auto& [name, info] : plugins_) {
            auto current_time = std::filesystem::last_write_time(info.library_path);
            if (current_time > info.last_modified) {
                reload_plugin(name, info);
            }
        }
    }
    
    std::unordered_map<std::string, PluginInfo> plugins_;
    std::vector<std::filesystem::path> watch_directories_;
    std::jthread reload_thread_;
    bool hot_reload_enabled_ = false;
};
```

## Performance Optimizations

### 1. Memory Pool Allocation

#### Current State
- Frequent malloc/free calls
- Memory fragmentation
- Cache misses

#### Proposed Changes
```cpp
class MemoryPool {
public:
    template<typename T>
    class PoolAllocator {
    public:
        using value_type = T;
        
        T* allocate(size_t n) {
            return static_cast<T*>(pool_->allocate(n * sizeof(T), alignof(T)));
        }
        
        void deallocate(T* p, size_t n) {
            pool_->deallocate(p, n * sizeof(T));
        }
        
    private:
        MemoryPool* pool_;
    };
    
    void* allocate(size_t size, size_t alignment);
    void deallocate(void* ptr, size_t size);
    void reset(); // Clear all allocations
    
private:
    std::vector<std::byte> buffer_;
    size_t current_offset_ = 0;
    std::vector<void*> free_blocks_;
};

// Usage in TypedDataArray
class TypedDataArray {
public:
    template<typename T>
    explicit TypedDataArray(const std::vector<T>& data, 
                           MemoryPool* pool = nullptr) {
        if (pool) {
            m_buffer = pool->allocate(data.size() * sizeof(T), alignof(T));
            m_pool = pool;
        } else {
            m_buffer = malloc(data.size() * sizeof(T));
        }
        // ... rest of construction
    }
    
private:
    MemoryPool* m_pool = nullptr; // Optional pool
};
```

### 2. SIMD-Optimized Operations

#### Current State
- Scalar operations in hot paths
- Missed vectorization opportunities

#### Proposed Changes
```cpp
#include <immintrin.h> // For AVX/SSE

class SIMDOperations {
public:
    static void apply_scale_offset_avx(double* data, size_t size, 
                                      double scale, double offset) {
        const __m256d scale_vec = _mm256_set1_pd(scale);
        const __m256d offset_vec = _mm256_set1_pd(offset);
        
        size_t simd_size = size & ~3; // Round down to multiple of 4
        
        for (size_t i = 0; i < simd_size; i += 4) {
            __m256d data_vec = _mm256_load_pd(&data[i]);
            data_vec = _mm256_fmadd_pd(data_vec, scale_vec, offset_vec);
            _mm256_store_pd(&data[i], data_vec);
        }
        
        // Handle remaining elements
        for (size_t i = simd_size; i < size; ++i) {
            data[i] = data[i] * scale + offset;
        }
    }
    
    // Dispatch based on CPU capabilities
    static void apply_scale_offset(double* data, size_t size,
                                  double scale, double offset) {
        if (cpu_supports_avx()) {
            apply_scale_offset_avx(data, size, scale, offset);
        } else if (cpu_supports_sse()) {
            apply_scale_offset_sse(data, size, scale, offset);
        } else {
            apply_scale_offset_scalar(data, size, scale, offset);
        }
    }
};
```

### 3. Lock-Free Data Structures

#### Current State
- Mutex-based synchronization
- Potential contention points
- Limited scalability

#### Proposed Changes
```cpp
#include <atomic>

template<typename T>
class LockFreeCacheEntry {
public:
    struct Node {
        std::atomic<T*> data{nullptr};
        std::atomic<Node*> next{nullptr};
        std::atomic<bool> deleted{false};
    };
    
    bool try_insert(T&& value) {
        Node* new_node = new Node;
        new_node->data.store(new T{std::move(value)});
        
        Node* expected = nullptr;
        return head_.compare_exchange_weak(expected, new_node);
    }
    
    std::optional<T> try_get() const {
        Node* current = head_.load();
        while (current) {
            if (!current->deleted.load()) {
                T* data = current->data.load();
                if (data) {
                    return *data;
                }
            }
            current = current->next.load();
        }
        return std::nullopt;
    }
    
private:
    std::atomic<Node*> head_{nullptr};
};
```

## API Evolution Strategy

### 1. Versioned APIs

```cpp
namespace libtokamap::v1 {
    // Current API for backward compatibility
    class MappingHandler {
        TypedDataArray map(const std::string& experiment,
                          const std::string& path,
                          std::type_index type,
                          int rank,
                          const nlohmann::json& attrs);
    };
}

namespace libtokamap::v2 {
    // New API with modern features
    template<typename T>
    class TypedMappingHandler {
        std::expected<TypedDataArray<T>, MappingError> 
        map(std::string_view experiment,
            std::string_view path,
            int rank,
            const ImmutableAttributes& attrs) const;
    };
}

// Compatibility layer
namespace libtokamap {
    using MappingHandler = v1::MappingHandler; // Default to v1
    
    // Migration helper
    template<typename T>
    auto upgrade_handler(v1::MappingHandler& old_handler) -> v2::TypedMappingHandler<T> {
        // Convert configuration and state
    }
}
```

### 2. Gradual Feature Migration

#### Phase 1: Infrastructure (3-6 months)
- [ ] Implement new type system with DataType enum
- [ ] Add std::format support for string operations
- [ ] Introduce concepts for template constraints
- [ ] Create immutable configuration system

#### Phase 2: Core Features (6-9 months)
- [ ] Add std::expected error handling
- [ ] Implement memory pool allocation
- [ ] Add SIMD-optimized operations
- [ ] Create async/coroutine support

#### Phase 3: Advanced Features (9-12 months)
- [ ] Plugin hot-reloading system
- [ ] Lock-free data structures
- [ ] Functional mapping composition
- [ ] Performance monitoring integration

## Migration Compatibility Matrix

| Feature | v1 Compatibility | Migration Effort | Breaking Changes |
|---------|------------------|------------------|------------------|
| Type System | Full | Low | None (additive) |
| std::expected | Full | Medium | None (new API) |
| Immutable Config | Partial | Medium | Constructor changes |
| std::format | Full | Low | None (internal) |
| Concepts | Full | Low | None (better errors) |
| Async APIs | Full | High | None (new API) |
| Memory Pools | Full | Medium | Optional feature |
| SIMD Ops | Full | Low | None (internal) |

## Code Quality Improvements

### 1. Static Analysis Integration

```cmake
# Enhanced CMakeLists.txt
option(ENABLE_STATIC_ANALYSIS "Enable static analysis tools" ON)

if(ENABLE_STATIC_ANALYSIS)
    find_program(CLANG_TIDY_EXE NAMES "clang-tidy")
    if(CLANG_TIDY_EXE)
        set(CLANG_TIDY_COMMAND 
            "${CLANG_TIDY_EXE}"
            "-checks=-*,modernize-*,performance-*,readability-*"
            "-header-filter=.*"
        )
        set_target_properties(libtokamap PROPERTIES
            CXX_CLANG_TIDY "${CLANG_TIDY_COMMAND}"
        )
    endif()
    
    find_program(CPPCHECK_EXE NAMES "cppcheck")
    if(CPPCHECK_EXE)
        add_custom_target(cppcheck
            COMMAND ${CPPCHECK_EXE}
            --enable=all
            --std=c++20
            --suppress=missingInclude
            ${CMAKE_SOURCE_DIR}/src
        )
    endif()
endif()
```

### 2. Comprehensive Testing Strategy

```cpp
// Property-based testing example
#include <catch2/catch.hpp>
#include <catch2/generators/catch_generators.hpp>

TEST_CASE("TypedDataArray operations are consistent", "[property]") {
    auto data_size = GENERATE(range(1, 1000));
    auto scale = GENERATE(range(-10.0, 10.0));
    auto offset = GENERATE(range(-5.0, 5.0));
    
    SECTION("Scale and offset operations commute with vector operations") {
        std::vector<double> original_data(data_size);
        std::iota(original_data.begin(), original_data.end(), 0.0);
        
        TypedDataArray array1{original_data};
        TypedDataArray array2{original_data};
        
        // Apply operations in different orders
        array1.apply<double>(scale, offset);
        auto result1 = array1.to_vector<double>();
        
        array2.apply<double>(1.0, offset);
        array2.apply<double>(scale, 0.0);
        auto result2 = array2.to_vector<double>();
        
        // Results should be equivalent within floating point precision
        REQUIRE(result1.size() == result2.size());
        for (size_t i = 0; i < result1.size(); ++i) {
            REQUIRE(std::abs(result1[i] - result2[i]) < 1e-10);
        }
    }
}
```

## Documentation and Developer Experience

### 1. Interactive Documentation

```cpp
// Example with embedded documentation tests
/**
 * @brief Maps experiment data to typed arrays
 * 
 * @example
 * ```cpp
 * MappingHandler handler{config};
 * auto result = handler.map<double>("ITER_001", "magnetics/flux", 2, {});
 * 
 * // Access data safely
 * if (result.has_value()) {
 *     auto data = result.value().to_vector<double>();
 *     process_magnetic_data(data);
 * }
 * ```
 * 
 * @doctest This example is tested automatically
 */
template<typename T>
std::expected<TypedDataArray<T>, MappingError>
map(std::string_view experiment, std::string_view path, 
    int rank, const Attributes& attrs) const;
```

### 2. Migration Tools

```bash
#!/bin/bash
# migrate_to_v2.sh - Automated migration script

echo "LibTokaMap v1 -> v2 Migration Tool"

# Find all usage patterns
echo "Scanning for v1 API usage..."
grep -r "MappingHandler.*map.*std::type_index" src/ --include="*.cpp" --include="*.hpp"

# Suggest replacements
echo "Suggested replacements:"
echo "  handler.map(exp, path, std::type_index{typeid(T)}, rank, attrs)"
echo "  -> handler.map<T>(exp, path, rank, attrs)"

# Generate migration patches
echo "Generating migration patches..."
# ... automated refactoring logic
```

## Risk Mitigation

### 1. Backwards Compatibility Testing

```cpp
// Compatibility test suite
class V1CompatibilityTest {
public:
    void test_all_v1_scenarios() {
        test_basic_mapping();
        test_data_source_registration();
        test_error_handling();
        test_configuration_loading();
    }
    
private:
    void test_basic_mapping() {
        // Ensure v1 API still works exactly as before
        libtokamap::v1::MappingHandler handler;
        // ... test implementation
    }
};
```

### 2. Performance Regression Detection

```cpp
#include <benchmark/benchmark.h>

static void BM_MappingPerformance_V1(benchmark::State& state) {
    libtokamap::v1::MappingHandler handler{config};
    for (auto _ : state) {
        auto result = handler.map("EXP", "path", type, rank, attrs);
        benchmark::DoNotOptimize(result);
    }
}

static void BM_MappingPerformance_V2(benchmark::State& state) {
    libtokamap::v2::TypedMappingHandler<double> handler{config};
    for (auto _ : state) {
        auto result = handler.map("EXP", "path", rank, attrs);
        benchmark::DoNotOptimize(result);
    }
}

BENCHMARK(BM_MappingPerformance_V1);
BENCHMARK(BM_MappingPerformance_V2);
```

## Implementation Roadmap

### Quarter 1: Foundation
- Set up new build system with static analysis
- Implement DataType enum and type system
- Add std::format support
- Create comprehensive test suite

### Quarter 2: Core Modernization  
- Implement concepts and template constraints
- Add std::expected error handling
- Create immutable configuration system
- Begin performance optimization work

### Quarter 3: Advanced Features
- Implement memory pool allocation
- Add SIMD-optimized operations
- Create async/coroutine support
- Begin API v2 development

### Quarter 4: Integration and Polish
- Plugin hot-reloading system
- Lock-free data structures
- Documentation and migration tools
- Performance testing and optimization

## Conclusion

This refactoring guide provides a structured approach to modernizing LibTokaMap while maintaining backward compatibility and improving performance. The key principles are:

1. **Gradual Migration**: Introduce new features alongside existing APIs
2. **Type Safety**: Leverage C++20 features for compile-time guarantees  
3. **Performance**: Focus on hot paths and memory efficiency
4. **Developer Experience**: Better APIs, documentation, and tooling
5. **Quality**: Comprehensive testing and static analysis

Success metrics:
- 100% backward compatibility maintained during transition
- >20% performance improvement in core operations
- Reduced compilation times with modules
- Improved developer productivity with better APIs
- Zero runtime crashes in production deployments