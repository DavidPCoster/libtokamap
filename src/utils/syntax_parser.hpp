#pragma once

#include <nlohmann/json.hpp>
#include <string>

namespace libtokamap
{

nlohmann::json expand_syntactic_sugar(nlohmann::json input);
std::string process_string_node(std::string value);

} // namespace libtokamap
