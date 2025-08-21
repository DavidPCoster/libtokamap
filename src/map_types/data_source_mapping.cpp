#include "data_source_mapping.hpp"

#include <inja/inja.hpp>
#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "map_types/map_arguments.hpp"
#include "utils/ram_cache.hpp"
#include "utils/render.hpp"
#include "utils/subset.hpp"

libtokamap::TypedDataArray libtokamap::DataSourceMapping::map(const MapArguments& arguments) const
{
    DataSourceArgs args = m_data_source_args;
    for (auto& [key, value] : args) {
        if (value.is_string()) {
            value = libtokamap::render(value.get<std::string>(), arguments.global_data);
        }
    }
    TypedDataArray array = m_data_source->get(args, arguments, m_ram_cache.get());

    // Render the slice string at runtime if it exists
    std::optional<std::string> rendered_slice;
    if (m_slice.has_value()) {
        rendered_slice = libtokamap::render(m_slice.value(), arguments.global_data);
    }

    update_array(array, rendered_slice, m_scale, m_offset);
    return array;
}

libtokamap::DataSourceMapping::DataSourceMapping(DataSource* data_source, DataSourceArgs data_source_args,
                                                 std::optional<float> offset, std::optional<float> scale,
                                                 std::optional<std::string> slice,
                                                 std::shared_ptr<libtokamap::RamCache> ram_cache)
    : m_data_source{data_source}, m_data_source_args{std::move(data_source_args)}, m_offset{offset}, m_scale{scale},
      m_slice{std::move(slice)}, m_ram_cache{std::move(ram_cache)}, m_cache_enabled(m_ram_cache != nullptr)
{
}
