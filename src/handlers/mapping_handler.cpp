#include "mapping_handler.hpp"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cxxabi.h>
#include <deque>
#include <filesystem>
#include <fstream>
#include <inja/inja.hpp>
#include <memory>
#include <nlohmann/json.hpp>
#include <optional>
#include <ostream>
#include <ranges>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <toml.hpp>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <valijson_nlohmann_bundled.hpp>
#include <vector>

#include "config_schema.hpp"
#include "exceptions/exceptions.hpp"
#include "map_types/base_mapping.hpp"
#include "map_types/custom_mapping.hpp"
#include "map_types/data_source_mapping.hpp"
#include "map_types/dim_mapping.hpp"
#include "map_types/expr_mapping.hpp"
#include "map_types/map_arguments.hpp"
#include "map_types/value_mapping.hpp"
#include "utils/algorithm.hpp"
#include "utils/indices.hpp"
#include "utils/library_loader.hpp"
#include "utils/mapping_locator.hpp"
#include "utils/ram_cache.hpp"
#include "utils/render.hpp"
#include "utils/syntax_parser.hpp"
#include "utils/types.hpp"

namespace
{

[[nodiscard]] valijson::Schema load_config_schema()
{
    nlohmann::json config_schema_json = nlohmann::json::parse(ConfigSchema);
    valijson::adapters::NlohmannJsonAdapter config_schema_adapter{config_schema_json};

    valijson::Schema config_schema;
    valijson::SchemaParser parser;
    parser.populateSchema(config_schema_adapter, config_schema);

    return config_schema;
}

[[nodiscard]] nlohmann::json load_json_file(const std::filesystem::path& path)
{
    std::ifstream file(path);
    if (!file.is_open()) {
        throw libtokamap::FileError{"Failed to open file: " + path.string()};
    }
    try {
        return nlohmann::json::parse(file);
    } catch (nlohmann::json::exception& ex) {
        throw libtokamap::JsonError{ex.what()};
    }
}

[[nodiscard]] nlohmann::json load_toml_file(const std::filesystem::path& file_path)
{
    std::ifstream file(file_path);
    if (!file.is_open()) {
        throw libtokamap::ConfigurationError{"Failed to open file: " + file_path.string()};
    }
    try {
        auto toml_config = toml::parse(file, file_path.string());
        std::stringstream stream;
        stream << toml::json_formatter(toml_config);
        return nlohmann::json::parse(stream.str());
    } catch (const toml::parse_error& e) {
        throw libtokamap::ConfigurationError{"Failed to parse TOML: " + std::string(e.what())};
    } catch (const nlohmann::json::parse_error& e) {
        throw libtokamap::ConfigurationError{"Failed to convert TOML to JSON: " + std::string(e.what())};
    }
}

void load_validation_schema(const nlohmann::json& config, const std::string& name, valijson::Schema& mapping_schema)
{
    if (!config.contains(name)) {
        throw libtokamap::ConfigurationError{name + " not specified in config"};
    }

    auto schema_path = config.at(name).get<std::string>();
    auto schema_json = load_json_file(schema_path);

    valijson::adapters::NlohmannJsonAdapter schema_adapter(schema_json);

    valijson::SchemaParser parser;
    parser.populateSchema(schema_adapter, mapping_schema);
}

void load_validation_schemas(const nlohmann::json& config, valijson::Schema& mapping_schema,
                             valijson::Schema& globals_schema, valijson::Schema& mapping_config_schema)
{
    load_validation_schema(config, "mapping_schema", mapping_schema);
    load_validation_schema(config, "globals_schema", globals_schema);
    load_validation_schema(config, "mapping_config_schema", mapping_config_schema);
}

void validate(const nlohmann::json& json, const valijson::Schema& schema)
{
    valijson::Validator validator;
    valijson::ValidationResults results;
    valijson::adapters::NlohmannJsonAdapter json_adapter{json};

    if (!validator.validate(schema, json_adapter, &results)) {
        std::stringstream msg;
        size_t error_num = 0;
        for (const auto& error : results) {
            msg << "Error #" << error_num << "\n ";
            for (const auto& message : error.context) {
                msg << message << " ";
            }
            msg << "\n";
            msg << " - " << error.description << "\n\n";
            ++error_num;
        }
        throw libtokamap::PathError{msg.str()};
    }
}

void uppercase_keys(nlohmann::json& data)
{
    for (auto& entry : data) {
        nlohmann::json new_entry;
        if (entry.is_object()) {
            for (const auto& [key, value] : entry.items()) {
                new_entry[libtokamap::to_upper_copy(key)] = value;
            }
            entry = new_entry;
        }
    }
}

[[nodiscard]] nlohmann::json load_json(const std::filesystem::path& file_path, const valijson::Schema& schema,
                                       bool to_upper = false)
{
    auto json = load_json_file(file_path);
    if (to_upper) {
        uppercase_keys(json);
    }
    validate(json, schema);
    return json;
}

[[nodiscard]] nlohmann::json load_toml(const std::filesystem::path& file_path, const valijson::Schema& schema)
{
    auto json = load_toml_file(file_path);
    validate(json, schema);
    return json;
}

struct MappingConfigMetadata {
    std::string experiment;
    std::string author;
    std::string version;
};

// inline void to_json(nlohmann::json& json, const MappingConfigMetadata& metadata)
// {
//     json["source"] = metadata.source;
//     json["author"] = metadata.author;
//     json["version"] = metadata.version;
// }

inline void from_json(const nlohmann::json& json, MappingConfigMetadata& metadata)
{
    json.at("experiment").get_to(metadata.experiment);
    json.at("author").get_to(metadata.author);
    json.at("version").get_to(metadata.version);
}

std::pair<libtokamap::ExperimentName, libtokamap::ExperimentMappings>
load_mapping_config(const std::filesystem::path& mapping_dir, const valijson::Schema& mapping_config_schema)
{
    auto file_path = mapping_dir / "mappings.cfg.json";
    auto mapping_config = load_json_file(file_path);
    validate(mapping_config, mapping_config_schema);

    auto metadata = mapping_config["metadata"].get<MappingConfigMetadata>();
    auto partition_list = mapping_config["partitions"].get<std::vector<libtokamap::MappingPartition>>();
    auto groups = mapping_config["groups"].get<std::vector<libtokamap::GroupName>>();

    libtokamap::ExperimentMappings experiment_mappings{partition_list, groups, mapping_dir};
    return {metadata.experiment, std::move(experiment_mappings)};
}

libtokamap::ExperimentRegisterStore locate_mappings(const std::filesystem::path& mapping_dir,
                                                    const valijson::Schema& mapping_config_schema)
{
    libtokamap::ExperimentRegisterStore experiment_register_store;
    for (const auto& directory : std::filesystem::directory_iterator{mapping_dir}) {
        auto [name, mapping] = load_mapping_config(directory, mapping_config_schema);
        experiment_register_store.emplace(name, std::move(mapping));
    }
    return experiment_register_store;
}

std::string find_mapping(const libtokamap::MappingStore& mappings, const std::string& path,
                         const std::vector<int>& indices, const std::string& full_path)
{
    // If mapping is found we are good
    if (mappings.contains(path)) {
        return path;
    }

    // Check with the path without generalisation
    if (mappings.contains(full_path)) {
        return full_path;
    }

    // If there's nothing to replace then no mapping can be found
    if (indices.empty()) {
        return "";
    }

    // Check for last # replaced with index
    std::string new_path = libtokamap::replace_last_copy(path, "#", std::to_string(indices.back()));
    if (mappings.contains(new_path)) {
        return new_path;
    }

    // No mappings found
    return "";
}

std::string generate_map_path(const std::deque<std::string>& path_tokens, const std::vector<int>& indices,
                              const libtokamap::MappingStore& mappings, const std::string& full_path)
{
    std::string map_path = libtokamap::join(path_tokens, "/");
    std::string found_path;

    if (!mappings.contains(map_path)) {
        found_path = find_mapping(mappings, map_path, indices, full_path);
    } else {
        found_path = map_path;
    }

    return found_path;
}

} // namespace

void libtokamap::MappingHandler::reset()
{
    m_experiment_register.clear();
    m_init = false;
}

libtokamap::TypedDataArray libtokamap::MappingHandler::map(const ExperimentName& experiment, const std::string& path,
                                                           std::type_index data_type, int rank,
                                                           const nlohmann::json& extra_attributes)
{
    std::deque<std::string_view> path_tokens;
    libtokamap::split(path_tokens, path, "/");
    if (path_tokens.empty()) {
        throw libtokamap::PathError{"Mapping path could not be split"};
    }

    auto [indices, new_tokens] = extract_indices(path_tokens);

    // Use first token of the mapping path as the group name
    const std::string group_name{new_tokens.front()};
    new_tokens.pop_front();

    // Use lowercase experiment name for find mapping files
    ExperimentName experiment_string = experiment;
    to_lower(experiment_string);

    if (!m_experiment_register.contains(experiment_string)) {
        auto msg = "no mappings found for experiment '" + experiment_string + "'";
        throw libtokamap::ParameterError{msg};
    }
    auto& experiment_mappings = m_experiment_register.at(experiment_string);
    load_experiment(experiment_string, extra_attributes);

    if (!experiment_mappings.group_mappings.contains(group_name)) {
        auto msg = "no mappings found for group '" + group_name + "'";
        throw libtokamap::ParameterError{msg};
    }
    auto& group_mappings = experiment_mappings.group_mappings.at(group_name);

    auto partition_attributes = find_partition_attributes(experiment_mappings.partition_list, extra_attributes);
    if (!group_mappings.contains(partition_attributes)) {
        auto msg = "no mappings found for partition " + partition_attributes.dump();
        throw libtokamap::ParameterError{msg};
    }
    auto& partition_mappings = group_mappings.at(partition_attributes);

    auto& [attributes, mappings] = partition_mappings;

    const std::string map_path = generate_map_path(new_tokens, indices, mappings, path);
    if (map_path.empty()) {
        throw libtokamap::MappingError{"failed to find mapping for '" + path + "'"};
    }

    // Add request indices to globals
    attributes["indices"] = indices;

    for (const auto& [key, value] : extra_attributes.items()) {
        attributes[key] = value;
    }

    const libtokamap::MapArguments map_arguments{mappings, attributes, data_type, rank};

    return mappings.at(map_path)->map(map_arguments);
}

namespace
{

void apply_config(std::unordered_map<std::string, nlohmann::json>& args, nlohmann::json plugin_config_map,
                  const std::string& plugin_name)
{
    if (plugin_config_map.contains(plugin_name)) {
        const auto& plugin_config = plugin_config_map[plugin_name].get<nlohmann::json>();
        const auto& plugin_args = plugin_config["ARGS"].get<nlohmann::json>();
        for (const auto& [name, arg] : plugin_args.items()) {
            if (!args.contains(name)) {
                // don't overwrite mapping arguments with global values
                args[name] = arg;
            }
        }
    }
}

std::optional<float> get_float_value(const std::string& name, const nlohmann::json& value,
                                     const nlohmann::json& group_attributes)
{
    std::optional<float> opt_float{std::nullopt};
    if (value.contains(name) and !value[name].is_null()) {
        if (value[name].is_number()) {
            opt_float = value[name].get<float>();
        } else if (value[name].is_string()) {
            try {
                const auto post_inja_str = libtokamap::render(value[name].get<std::string>(), group_attributes);
                opt_float = std::stof(post_inja_str);
            } catch (const std::invalid_argument&) {
                // const std::string message = "\nCannot convert " + name + " string to float\n";
                // UDA_LOG(UDA_LOG_DEBUG, "%s", message.c_str());
            }
        }
    }
    return opt_float;
}

void init_value_mapping(libtokamap::MappingStore& map_store, const libtokamap::MappingName& mapping_name,
                        const nlohmann::json& value)
{
    const auto& value_json = value.at("VALUE");
    map_store.try_emplace(mapping_name, std::make_unique<libtokamap::ValueMapping>(value_json));
}

void init_data_source_mapping(libtokamap::MappingStore& map_store, const libtokamap::MappingName& mapping_name,
                              const nlohmann::json& value, const nlohmann::json& group_attributes,
                              std::shared_ptr<libtokamap::RamCache>& ram_cache,
                              const libtokamap::DataSourceRegistry& data_sources)
{
    if (!value.contains("DATA_SOURCE")) {
        throw libtokamap::ConfigurationError{"required DATA_SOURCE argument not provided in DATA_SOURCE mapping '" +
                                             mapping_name + "'"};
    }
    std::string data_source_name = value["DATA_SOURCE"].get<std::string>();
    libtokamap::to_upper(data_source_name);

    if (!value.contains("ARGS")) {
        throw libtokamap::ConfigurationError{"required ARGS argument not provided in DATA_SOURCE mapping '" +
                                             mapping_name + "'"};
    }
    auto args = value["ARGS"].get<libtokamap::DataSourceArgs>();
    auto offset = get_float_value("OFFSET", value, group_attributes);
    auto scale = get_float_value("SCALE", value, group_attributes);
    auto slice = value.contains("SLICE") ? std::optional<std::string>{value.at("SLICE").get<std::string>()}
                                         : std::optional<std::string>{};

    if (group_attributes.contains("DATA_SOURCE_CONFIG")) {
        const auto& plugin_config_map = group_attributes.at("DATA_SOURCE_CONFIG");
        apply_config(args, plugin_config_map, data_source_name);
    }

    if (!data_sources.contains(data_source_name)) {
        throw libtokamap::DataSourceError{"data source " + data_source_name + " not registered"};
    }
    auto* data_source = data_sources.at(data_source_name).get();

    map_store.try_emplace(mapping_name, std::make_unique<libtokamap::DataSourceMapping>(data_source, args, offset,
                                                                                        scale, slice, ram_cache));
}

void init_dim_mapping(libtokamap::MappingStore& map_store, const libtokamap::MappingName& mapping_name,
                      const nlohmann::json& value)
{
    map_store.try_emplace(mapping_name,
                          std::make_unique<libtokamap::DimMapping>(value["DIM_PROBE"].get<std::string>()));
}

void init_expr_mapping(libtokamap::MappingStore& map_store, const libtokamap::MappingName& mapping_name,
                       const nlohmann::json& value)
{
    map_store.try_emplace(mapping_name, std::make_unique<libtokamap::ExprMapping>(
                                            value["EXPR"].get<std::string>(),
                                            value["PARAMETERS"].get<std::unordered_map<std::string, std::string>>()));
}

void init_custom_mapping(libtokamap::MappingStore& map_store, const libtokamap::MappingName& mapping_name,
                         const nlohmann::json& value, const std::vector<libtokamap::LibraryFunction>& library_functions)
{
    std::vector<std::filesystem::path> library_paths = {};
    auto library_name = value["LIBRARY"].get<libtokamap::LibraryName>();
    auto function_name = value["FUNCTION"].get<libtokamap::FunctionName>();
    auto input_map = value["INPUTS"].get<libtokamap::CustomMappingInputMap>();
    auto params = value["PARAMETERS"];
    map_store.try_emplace(mapping_name, std::make_unique<libtokamap::CustomMapping>(library_functions, library_name,
                                                                                    function_name, input_map, params));
}

void parse_globals(nlohmann::json& globals)
{
    // expand syntactic sugar
    for (const auto& [key, value] : globals.items()) {
        if (value.is_string()) {
            value = libtokamap::process_string_node(value);
        }
    }
}

void parse_mappings(nlohmann::json& mappings)
{
    // expand syntactic sugar
    for (const auto& [key, value] : mappings.items()) {
        value = libtokamap::parse(value);
    }
}

} // namespace

libtokamap::MappingStore libtokamap::MappingHandler::init_mappings(const nlohmann::json& data,
                                                                   const nlohmann::json& group_attributes)
{
    // const auto& attributes = m_experiment_register[experiment].group_globals[group_name];
    libtokamap::MappingStore map_store;
    for (const auto& [mapping_name, value] : data.items()) {
        if (!value.contains("MAP_TYPE")) {
            throw libtokamap::MappingError{"required MAP_TYPE argument not found in mapping '" + mapping_name + "'"};
        }

        // TODO: make this case insensitive?
        using libtokamap::MappingType;
        switch (value["MAP_TYPE"].get<MappingType>()) {
            case MappingType::VALUE:
                init_value_mapping(map_store, mapping_name, value);
                break;
            case MappingType::DATA_SOURCE:
                init_data_source_mapping(map_store, mapping_name, value, group_attributes, m_ram_cache, m_data_sources);
                break;
            case MappingType::DIM:
                init_dim_mapping(map_store, mapping_name, value);
                break;
            case MappingType::EXPR:
                init_expr_mapping(map_store, mapping_name, value);
                break;
            case MappingType::CUSTOM:
                init_custom_mapping(map_store, mapping_name, value, m_library_functions);
                break;
            default:
                break;
        }
    }
    return map_store;
}

void libtokamap::MappingHandler::init(const std::filesystem::path& config_path)
{
    auto config_schema = load_config_schema();

    nlohmann::json config;
    if (config_path.extension() == ".json") {
        config = load_json(config_path, config_schema);
    } else if (config_path.extension() == ".toml") {
        config = load_toml(config_path, config_schema);
    } else {
        throw libtokamap::ConfigurationError{"Unsupported configuration file type"};
    }
    init(config);
}

void libtokamap::MappingHandler::init(const nlohmann::json& config)
{
    auto config_schema = load_config_schema();
    validate(config, config_schema);

    if (m_init || !m_experiment_register.empty()) {
        return;
    }

    load_validation_schemas(config, m_mappings_schema, m_globals_schema, m_mapping_config_schema);

    if (!config.contains("mapping_directory")) {
        throw libtokamap::ConfigurationError{"mapping_directory not specified in config"};
    }
    m_mapping_dir = config.at("mapping_directory").get<std::string>();
    m_experiment_register = locate_mappings(m_mapping_dir, m_mapping_config_schema);

    bool enable_caching = config.contains("cache_enabled") && config.at("cache_enabled").get<bool>();

    if (enable_caching) {
        const std::size_t cache_size =
            config.contains("cache_size") ? config.at("cache_size").get<int>() : libtokamap::default_size;
        m_ram_cache = std::make_shared<libtokamap::RamCache>(cache_size);
    } else {
        m_ram_cache = nullptr;
    }

    m_cache_enabled = m_ram_cache != nullptr;
    m_init = true;

    if (config.contains("custom_library_paths")) {
        std::vector<std::filesystem::path> paths =
            config.at("custom_library_paths").get<std::vector<std::filesystem::path>>();
        m_library_functions = load_libraries(paths);
    }
}

void libtokamap::MappingHandler::load_experiment(const ExperimentName& experiment, const nlohmann::json& attributes)
{
    if (!m_experiment_register.contains(experiment)) {
        throw MappingError{"Experiment '" + experiment + "' not found in mappings"};
    }

    if (m_experiment_register[experiment].is_loaded) {
        // experiment already loaded
        return;
    }

    auto& experiment_mapping = m_experiment_register[experiment];

    const auto& mapping_dir = experiment_mapping.root_path;

    auto top_level_globals = load_json(mapping_dir / "globals.json", m_globals_schema);
    parse_globals(top_level_globals);
    validate(top_level_globals, m_globals_schema);
    experiment_mapping.top_level_globals = top_level_globals;

    auto& partition_list = experiment_mapping.partition_list;

    for (const auto& group_name : experiment_mapping.groups) {
        auto group_directory = mapping_dir / group_name;
        auto partition_directory = find_partition_directory(group_directory, partition_list, attributes);
        auto partition_attributes = find_partition_attributes(partition_list, attributes);

        MappingPair mapping_pair;

        mapping_pair.globals = load_json(partition_directory / "globals.json", m_globals_schema);
        parse_globals(mapping_pair.globals);
        validate(mapping_pair.globals, m_globals_schema);
        mapping_pair.globals.update(top_level_globals);

        constexpr bool to_upper = true;
        auto mappings_json = load_json(partition_directory / "mappings.json", m_mappings_schema, to_upper);
        parse_mappings(mappings_json);
        validate(mappings_json, m_mappings_schema);
        mapping_pair.mappings = init_mappings(mappings_json, mapping_pair.globals);

        experiment_mapping.group_mappings[group_name][partition_attributes] = std::move(mapping_pair);
    }

    experiment_mapping.is_loaded = true;
}

void libtokamap::MappingHandler::register_data_source(const std::string& name, std::unique_ptr<DataSource> data_source)
{
    m_data_sources[name] = std::move(data_source);
}

void libtokamap::MappingHandler::unregister_data_source(const std::string& name) { m_data_sources.erase(name); }

void libtokamap::MappingHandler::register_custom_function(LibraryFunction custom_function)
{
    m_library_functions.emplace_back(std::move(custom_function));
}

void libtokamap::MappingHandler::unregister_custom_function(const std::string& library_name,
                                                            const std::string& function_name)
{
    auto element =
        std::ranges::find_if(m_library_functions, [&library_name, &function_name](const LibraryFunction& func) {
            return func.matches(library_name, function_name);
        });
    if (element != m_library_functions.end()) {
        m_library_functions.erase(element);
    } else {
        throw std::runtime_error("Custom function not found");
    }
}
