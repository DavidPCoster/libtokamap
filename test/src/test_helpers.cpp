#include "test_helpers.hpp"

#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <typeindex>
#include <unordered_map>

#include "map_types/base_mapping.hpp"
#include "map_types/map_arguments.hpp"
#include "utils/ram_cache.hpp"

libtokamap::MapArguments makeMapArguments(const std::type_index data_type, const int rank)
{
    static std::unordered_map<std::string, std::unique_ptr<libtokamap::Mapping>> empty_entries;
    static nlohmann::json empty_global_data = nlohmann::json::object();

    constexpr bool trace_enabled = true;
    constexpr bool cache_enabled = false;
    constexpr libtokamap::RamCache* ram_cache = nullptr;

    return libtokamap::MapArguments(empty_entries, empty_global_data, data_type, rank, trace_enabled, cache_enabled,
                                    ram_cache);
}
