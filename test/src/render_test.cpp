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

TEST_CASE("Test render conditionals", "[render]") {
    std::string input = "{% if existsIn(at(FLUX_LOOPS, indices.0), \"LOOPV\") %}{{ at(FLUX_LOOPS, indices.0).LOOPV }}{% endif %}";
    std::vector<inja::json> flux_loops = {
        { { "LOOP_NAME", "loop1" }, { "LOOPV", "loopv1" } },
        { { "LOOP_NAME", "loop2" } },
    };
    inja::json globals = {
        {"FLUX_LOOPS", flux_loops},
    };

    globals["indices"] = {0};
    std::string actual = render(input, globals);
    std::string expected = "loopv1";
    REQUIRE(actual == expected);

    globals["indices"] = {1};
    actual = render(input, globals);
    expected = "";
    REQUIRE(actual == expected);
}

TEST_CASE("Test optional key", "[render]") {
    inja::Environment env;

    env.add_callback("optional", 2, [](inja::Arguments& args) {
        auto json = args.at(0)->get<inja::json>();
        auto key = args.at(1)->get<std::string>();
        if (json.contains(key)) {
            return json[key];
        }
        return inja::json{};
    });

    std::string input = "{{ optional(at(FLUX_LOOPS, indices.0), \"LOOPV\") }}";
    std::vector<inja::json> flux_loops = {
        { { "LOOP_NAME", "loop1" }, { "LOOPV", "loopv1" } },
        { { "LOOP_NAME", "loop2" } },
    };
    inja::json globals = {
        {"FLUX_LOOPS", flux_loops},
    };

    globals["indices"] = {0};
    std::string actual = env.render(input, globals);
    std::string expected = "loopv1";
    REQUIRE(actual == expected);

    globals["indices"] = {1};
    actual = env.render(input, globals);
    expected = "";
    REQUIRE(actual == expected);
}
