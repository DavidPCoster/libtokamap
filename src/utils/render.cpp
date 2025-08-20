#include "render.hpp"

#include <exception>
#include <inja/inja.hpp>
#include <string>

#include "exceptions/exceptions.hpp"

std::string libtokamap::render(const std::string& template_string, const inja::json& data)
{
    try {
        return inja::render(inja::render(template_string, data), data);
    } catch (const std::exception& e) {
        throw libtokamap::TokaMapError(e.what());
    }
}
