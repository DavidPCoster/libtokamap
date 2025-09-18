#pragma once

#include <cstdlib>
#include <ctime>
#include <optional>
#include <string>
#include <vector>

#include "utils/typed_data_array.hpp"

namespace libtokamap
{

std::vector<libtokamap::SubsetInfo> parse_slices(const std::string& slice, const std::vector<size_t>& shape);

void update_array(TypedDataArray& input, const std::optional<std::string>& slice, std::optional<float> scale_factor,
                  std::optional<float> offset);

} // namespace libtokamap
