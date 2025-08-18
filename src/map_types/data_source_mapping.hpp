#pragma once

#include "base_mapping.hpp"
#include "map_types/map_arguments.hpp"
#include "utils/ram_cache.hpp"

#include <memory>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <unordered_map>

namespace libtokamap
{

using DataSourceArgs = std::unordered_map<std::string, nlohmann::json>;

class DataSource
{
  public:
    DataSource() = default;
    virtual TypedDataArray get(const DataSourceArgs& map_args, const MapArguments& arguments, RamCache* ram_cache) = 0;
    virtual ~DataSource() = default;

    DataSource(DataSource&& other) = default;
    DataSource(const DataSource& other) = default;
    DataSource& operator=(DataSource&& other) = default;
    DataSource& operator=(const DataSource& other) = default;
};

class DataSourceMapping : public Mapping
{
  public:
    DataSourceMapping() = delete;
    DataSourceMapping(DataSource* m_data_source, DataSourceArgs data_source_args, std::optional<float> offset,
                      std::optional<float> scale, std::optional<std::string> slice,
                      std::shared_ptr<libtokamap::RamCache> ram_cache);

    [[nodiscard]] TypedDataArray map(const MapArguments& arguments) const override;

  private:
    DataSource* m_data_source;
    DataSourceArgs m_data_source_args;
    std::optional<float> m_offset;
    std::optional<float> m_scale;
    std::optional<std::string> m_slice;
    std::shared_ptr<libtokamap::RamCache> m_ram_cache;
    bool m_cache_enabled;
};

} // namespace libtokamap
