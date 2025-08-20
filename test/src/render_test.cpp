#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>
#include <vector>
#include <string>

#include "inja/inja.hpp"
#include "utils/render.hpp"

using namespace libtokamap;

TEST_CASE("Test double render", "[render]") {
    std::string input = "{{ FL_NAME }}";
    std::string expected = "loop1";
    std::vector<inja::json> flux_loops = {
        { { "LOOP_NAME", "loop1" } },
        { { "LOOP_NAME", "loop2" } },
        { { "LOOP_NAME", "loop3" } }
    };
    std::vector<int> indices = {0};
    inja::json globals = {
        {"indices", indices},
        {"FLUX_LOOPS", flux_loops},
        {"FL_NAME", "{{ at(FLUX_LOOPS, indices.0).LOOP_NAME }}"}
    };
    std::string actual = render(input, globals);
    REQUIRE(actual == expected);
}
