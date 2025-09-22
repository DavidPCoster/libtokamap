#pragma once

#include <chrono>
#include <fstream>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace libtokamap
{

class Profiler
{
  public:
    explicit Profiler(std::string function_name)
        : m_function_name{std::move(function_name)}, m_start_time{std::chrono::steady_clock::now()}
    {
    }
    ~Profiler()
    {
        auto now = std::chrono::steady_clock::now();
        m_timings[m_function_name].emplace_back(m_start_time, now, m_attributes);
    }
    Profiler(const Profiler&) = delete;
    Profiler& operator=(const Profiler&) = delete;
    Profiler(Profiler&&) = delete;
    Profiler& operator=(Profiler&&) = delete;

    nlohmann::json& operator[](const std::string& name) { return m_attributes[name]; }

    static void init() {}
    static void write(const std::string& filename)
    {
        nlohmann::json json;
        for (const auto& [function, samples] : m_timings) {
            json[function] = nlohmann::json::array();
            for (const auto& [start, end, attributes] : samples) {
                json[function].push_back({
                    {"duration", std::chrono::duration_cast<std::chrono::microseconds>(end - start).count()},
                    {"attributes", attributes}
                });
            }
        }

        if (!filename.empty()) {
            std::ofstream file(filename);
            file << json.dump(4);
        }
    }

  private:
    struct Sample {
        std::chrono::time_point<std::chrono::steady_clock> start_time;
        std::chrono::time_point<std::chrono::steady_clock> end_time;
        std::unordered_map<std::string, nlohmann::json> attributes;
    };

    static std::unordered_map<std::string, std::vector<Sample>> m_timings;

    std::string m_function_name;
    std::chrono::time_point<std::chrono::steady_clock> m_start_time;
    std::unordered_map<std::string, nlohmann::json> m_attributes;
};

} // namespace libtokamap

#ifdef LIBTOKAMAP_PROFILE_ENABLED
#  define LIBTOKAMAP_PROFILER(NAME) libtokamap::Profiler NAME{__PRETTY_FUNCTION__};
#  define LIBTOKAMAP_PROFILER_ATTR(NAME, ATTR, VALUE) NAME[ATTR] = VALUE;
#else
#  define LIBTOKAMAP_PROFILER(NAME)
#  define LIBTOKAMAP_PROFILER_ATTR(NAME, ATTR, VALUE)
#endif
