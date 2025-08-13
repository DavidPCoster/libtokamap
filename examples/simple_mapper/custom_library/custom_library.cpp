#include <libtokamap.hpp>

#include <cstddef>
#include <span>
#include <typeindex>
#include <vector>

#include "utils/library_loader.hpp"

namespace
{

libtokamap::TypedDataArray dot_product(libtokamap::CustomMappingInputs& inputs,
                                       const libtokamap::CustomMappingParams& /*params*/)
{
    if (!inputs.contains("lhs")) {
        throw libtokamap::TokaMapError("dot_product expects input 'lhs'");
    }
    if (!inputs.contains("rhs")) {
        throw libtokamap::TokaMapError("dot_product expects input 'rhs'");
    }
    const auto& lhs = inputs["lhs"];
    const auto& rhs = inputs["rhs"];

    if (lhs.size() != rhs.size()) {
        throw libtokamap::TokaMapError("Vectors must have the same size");
    }
    if (lhs.rank() != 1 || rhs.rank() != 1) {
        throw libtokamap::TokaMapError("Vectors must be 1-dimensional");
    }
    if (lhs.type_index() != std::type_index{typeid(float)} || rhs.type_index() != std::type_index{typeid(float)}) {
        throw libtokamap::TokaMapError("Vectors must be of type float");
    }

    const auto lhs_data = lhs.span<float>();
    const auto rhs_data = rhs.span<float>();
    float result = 0.0F;

    for (size_t i = 0; i < lhs.size(); ++i) {
        result += lhs_data[i] * rhs_data[i];
    }

    return libtokamap::TypedDataArray{result};
}

} // namespace

extern "C" void LibTokaMapEntry(libtokamap::LibraryEntryInterface& interface)
{
    interface.functions.emplace_back("custom", "dot_product", dot_product);
}
