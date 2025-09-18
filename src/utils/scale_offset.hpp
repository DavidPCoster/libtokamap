#pragma once

#include "utils/typed_data_array.hpp"

namespace libtokamap::map_transform
{

int transform_offset(TypedDataArray& array, float offset);
int transform_scale(TypedDataArray& array, float scale);

} // namespace libtokamap::map_transform
