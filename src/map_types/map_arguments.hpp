#pragma once

#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <typeindex>
#include <unordered_map>

#include "utils/ram_cache.hpp"

namespace libtokamap
{

class Mapping;

struct MapArguments {
    const std::unordered_map<std::string, std::unique_ptr<Mapping>>& entries;
    const nlohmann::json& global_data;
    std::type_index data_type;
    int rank;
    bool trace_enabled;
    bool cache_enabled;
    RamCache* ram_cache;

    explicit MapArguments(const std::unordered_map<std::string, std::unique_ptr<Mapping>>& entries,
                          const nlohmann::json& global_data, const std::type_index data_type, const int rank,
                          const bool trace_enabled, const bool cache_enabled, RamCache* ram_cache)
        : entries{entries}, global_data{global_data}, data_type{data_type}, rank{rank}, trace_enabled{trace_enabled},
          cache_enabled{cache_enabled}, ram_cache{ram_cache}
    {
    }
};

} // namespace libtokamap
