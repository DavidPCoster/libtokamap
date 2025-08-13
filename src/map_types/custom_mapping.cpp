#include "map_types/custom_mapping.hpp"

#include <filesystem>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "exceptions/exceptions.hpp"
#include "map_types/map_arguments.hpp"
#include "utils/library_loader.hpp"

libtokamap::CustomMapping::CustomMapping(const std::vector<libtokamap::LibraryFunction>& functions,
                                         const libtokamap::LibraryName& library_name,
                                         const libtokamap::FunctionName& function_name, CustomMappingInputMap input_map,
                                         CustomMappingParams params)
    : m_input_map(std::move(input_map)), m_params(std::move(params))
{
    bool found = false;
    for (const auto& function : functions) {
        if (function.library_name == library_name && function.function_name == function_name) {
            m_function = function;
            found = true;
            break;
        }
    }
    if (!found) {
        throw libtokamap::TokaMapError("Function '" + function_name + "' not found in library '" + library_name + "'");
    }
};

libtokamap::TypedDataArray libtokamap::CustomMapping::map(const MapArguments& arguments) const
{
    CustomMappingInputs inputs;
    for (const auto& [name, mapping] : m_input_map) {
        if (!arguments.entries.contains(mapping)) {
            throw libtokamap::TokaMapError("Input '" + mapping + "' not found in mappings");
        }
        const auto& input = arguments.entries.at(mapping);
        inputs.insert({name, input->map(arguments)});
    }

    return m_function.function(inputs, m_params);
}
