#pragma once

#include <inja/inja.hpp>
#include <string>

namespace libtokamap
{

std::string render(const std::string& template_string, const inja::json& data);

} // namespace libtokamap
