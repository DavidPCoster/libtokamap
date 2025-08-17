#pragma once

#include <cstddef>
#include <filesystem>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <vector>

#include "map_types/map_arguments.hpp"
#include "utils/library_loader.hpp"
#include "utils/ram_cache.hpp"
#include "utils/types.hpp"
#include "valijson_nlohmann_bundled.hpp"

struct PluginList;

namespace libtokamap
{

class DataSource;

using DataSourceRegistry = std::unordered_map<std::string, std::unique_ptr<libtokamap::DataSource>>;

class MappingHandler
{
  public:
    MappingHandler() = default;

    void reset();
    void init(const nlohmann::json& config);

    [[nodiscard]] TypedDataArray map(const std::string& experiment, const std::string& path, std::type_index data_type,
                                     int rank, const nlohmann::json& extra_attributes);

    void register_data_source(const std::string& name, std::unique_ptr<DataSource> data_source);
    void unregister_data_source(const std::string& name);

    void register_custom_function(LibraryFunction custom_function);
    void unregister_custom_function(const std::string& library_name, const std::string& function_name);

  private:
    void load_experiment(const ExperimentName& experiment, const nlohmann::json& attributes);
    libtokamap::MappingStore init_mappings(const nlohmann::json& data, const nlohmann::json& group_attributes);

    DataSourceRegistry m_data_sources;
    ExperimentRegisterStore m_experiment_register;
    bool m_init = false;

    std::string m_dd_version;
    std::filesystem::path m_mapping_dir;
    std::shared_ptr<libtokamap::RamCache> m_ram_cache;
    bool m_cache_enabled = false;
    valijson::Schema m_mappings_schema;
    valijson::Schema m_globals_schema;
    valijson::Schema m_mapping_config_schema;
    std::vector<libtokamap::LibraryFunction> m_library_functions;
};

} // namespace libtokamap
