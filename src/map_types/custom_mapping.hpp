#pragma once

#include <filesystem>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>
#include <vector>

#include "map_types/base_mapping.hpp"
#include "map_types/map_arguments.hpp"
#include "utils/library_loader.hpp"

namespace libtokamap
{

using CustomMappingInputMap = std::unordered_map<std::string, std::string>;

class CustomMapping : public Mapping
{
  public:
    CustomMapping() = delete;
    ~CustomMapping() override = default;
    explicit CustomMapping(const std::vector<LibraryFunction>& functions, const LibraryName& library_name,
                           const FunctionName& function_name, CustomMappingInputMap input_map,
                           CustomMappingParams params);

    CustomMapping(CustomMapping&& other) = default;
    CustomMapping(const CustomMapping& other) = delete;
    CustomMapping& operator=(CustomMapping&& other) = default;
    CustomMapping& operator=(const CustomMapping& other) = delete;

    [[nodiscard]] TypedDataArray map(const MapArguments& arguments) const override;

  private:
    std::string m_function_name;
    const LibraryFunction* m_function;
    CustomMappingInputMap m_input_map;
    CustomMappingParams m_params;
};

} // namespace libtokamap
