#include "profiler.hpp"

#include <string>
#include <unordered_map>
#include <vector>

std::unordered_map<std::string, std::vector<libtokamap::Profiler::Sample>> libtokamap::Profiler::m_timings = {};
