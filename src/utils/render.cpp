#include "render.hpp"

#include <inja/inja.hpp>
#include <string>

std::string libtokamap::render(const std::string& template_string, const inja::json& data)
{
    return inja::render(inja::render(template_string, data), data);
}
