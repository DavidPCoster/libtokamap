#pragma once

#include <filesystem>
#include <functional>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>
#include <vector>

#include "map_types/map_arguments.hpp"

namespace libtokamap
{

using CustomMappingInputs = std::unordered_map<std::string, libtokamap::TypedDataArray>;
using CustomMappingParams = nlohmann::json;

using LibraryName = std::string;
using FunctionName = std::string;

using FunctionType = std::function<TypedDataArray(CustomMappingInputs&, const CustomMappingParams&)>;

struct LibraryFunction {
    LibraryName library_name;
    FunctionName function_name;
    FunctionType function;
};

struct LibraryEntryInterface {
    std::vector<LibraryFunction> functions;
};

std::vector<libtokamap::LibraryFunction> load_libraries(std::vector<std::filesystem::path>& library_paths);

} // namespace libtokamap
