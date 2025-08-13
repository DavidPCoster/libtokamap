#include "library_loader.hpp"

#include <dlfcn.h>
#include <filesystem>
#include <string>
#include <vector>

#include "exceptions/exceptions.hpp"

namespace
{

using EntryFunction = void (*)(libtokamap::LibraryEntryInterface&);

constexpr const char* function_name = "LibTokaMapEntry";

std::vector<libtokamap::LibraryFunction> load_library_functions(const std::filesystem::path& library_path)
{
    void* handle = dlopen(library_path.c_str(), RTLD_LAZY);
    if (handle == nullptr) {
        throw libtokamap::TokaMapError("Failed to load library '" + library_path.string() + "'");
    }

    void* function_pointer = dlsym(handle, function_name);
    if (function_pointer == nullptr) {
        throw libtokamap::TokaMapError("Failed to load entry function");
    }

    auto entry_function = reinterpret_cast<EntryFunction>(function_pointer);
    libtokamap::LibraryEntryInterface entry_interface;
    entry_function(entry_interface);

    return entry_interface.functions;
}

} // namespace

std::vector<libtokamap::LibraryFunction> libtokamap::load_libraries(std::vector<std::filesystem::path>& library_paths)
{
    std::vector<libtokamap::LibraryFunction> library_functions;
    for (auto& path : library_paths) {
        if (!std::filesystem::exists(path)) {
            throw libtokamap::TokaMapError("Library path '" + path.string() + "' does not exist");
        }
        if (!std::filesystem::is_directory(path)) {
            throw libtokamap::TokaMapError("Invalid library path '" + path.string() + "'");
        }
        for (const auto& entry : std::filesystem::directory_iterator(path)) {
            if (entry.is_regular_file() && entry.path().extension() == ".dylib") {
                auto functions = load_library_functions(entry);
                library_functions.insert(library_functions.end(), functions.begin(), functions.end());
            }
        }
    }
    return library_functions;
}
