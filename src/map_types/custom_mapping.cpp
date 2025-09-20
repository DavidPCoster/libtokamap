#include "map_types/custom_mapping.hpp"

#include <algorithm>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "exceptions/exceptions.hpp"
#include "map_types/map_arguments.hpp"
#include "utils/library_loader.hpp"
#include "utils/profiler.hpp"
#include "utils/typed_data_array.hpp"

libtokamap::CustomMapping::CustomMapping(const std::vector<libtokamap::LibraryFunction>& functions,
                                         const libtokamap::LibraryName& library_name,
                                         const libtokamap::FunctionName& function_name, CustomMappingInputMap input_map,
                                         CustomMappingParams params)
    : m_function_name{function_name}, m_input_map(std::move(input_map)), m_params(std::move(params))
{
    auto found = std::ranges::find_if(functions, [&](auto& func) { return func.matches(library_name, function_name); });
    if (found == functions.end()) {
        throw libtokamap::TokaMapError("Function '" + function_name + "' not found in library '" + library_name + "'");
    }
    m_function = &*found;
};

libtokamap::TypedDataArray libtokamap::CustomMapping::map(const MapArguments& arguments) const
{
    LIBTOKAMAP_PROFILER(profiler);

    CustomMappingInputs inputs;
    for (const auto& [name, mapping] : m_input_map) {
        if (!arguments.entries.contains(mapping)) {
            throw libtokamap::TokaMapError("Input '" + mapping + "' not found in mappings");
        }
        const auto& input = arguments.entries.at(mapping);
        inputs.insert({name, input->map(arguments)});
    }

    auto result = m_function->call(inputs, m_params);
    if (arguments.trace_enabled) {
        nlohmann::json inputs_trace;
        for (const auto& [name, input] : inputs) {
            inputs_trace[name] = input.trace();
        }
        result.set_trace({{"map_type", "custom_function"}, {"function", m_function_name}, {"inputs", inputs_trace}});
    }
    return result;
}
