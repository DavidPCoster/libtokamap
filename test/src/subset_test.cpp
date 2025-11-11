#include <catch2/catch_test_macros.hpp>

#include "utils/subset.hpp"
#include "utils/typed_data_array.hpp"

using namespace libtokamap;

TEST_CASE("Subset 2D to 1D array", "[subset]")
{
    constexpr size_t rows = 10;
    constexpr size_t cols = 15;

    // we row-major or column-major?! Is the libtokamap subsetting expecting row-major?!
    // Standard in c++ so I think so... need to check, that is the assumption here
    // ** Switched to hardcoded arrays just for simpilicity
    // 10x15
    std::vector<float> data = {
        0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0, 12.0, 13.0, 14.0,
        100.0, 101.0, 102.0, 103.0, 104.0, 105.0, 106.0, 107.0, 108.0, 109.0, 110.0, 111.0, 112.0, 113.0, 114.0,
        200.0, 201.0, 202.0, 203.0, 204.0, 205.0, 206.0, 207.0, 208.0, 209.0, 210.0, 211.0, 212.0, 213.0, 214.0,
        300.0, 301.0, 302.0, 303.0, 304.0, 305.0, 306.0, 307.0, 308.0, 309.0, 310.0, 311.0, 312.0, 313.0, 314.0,
        400.0, 401.0, 402.0, 403.0, 404.0, 405.0, 406.0, 407.0, 408.0, 409.0, 410.0, 411.0, 412.0, 413.0, 414.0,
        500.0, 501.0, 502.0, 503.0, 504.0, 505.0, 506.0, 507.0, 508.0, 509.0, 510.0, 511.0, 512.0, 513.0, 514.0,
        600.0, 601.0, 602.0, 603.0, 604.0, 605.0, 606.0, 607.0, 608.0, 609.0, 610.0, 611.0, 612.0, 613.0, 614.0,
        700.0, 701.0, 702.0, 703.0, 704.0, 705.0, 706.0, 707.0, 708.0, 709.0, 710.0, 711.0, 712.0, 713.0, 714.0,
        800.0, 801.0, 802.0, 803.0, 804.0, 805.0, 806.0, 807.0, 808.0, 809.0, 810.0, 811.0, 812.0, 813.0, 814.0,
        900.0, 901.0, 902.0, 903.0, 904.0, 905.0, 906.0, 907.0, 908.0, 909.0, 910.0, 911.0, 912.0, 913.0, 914.0
    };

    SECTION("Select single column [:][9]")
    {
        TypedDataArray array{data, {rows, cols}};

        // Parse the slice string [:][9]
        auto subsets = parse_slices("[:][9]", array.shape());

        REQUIRE(subsets.size() == 2);
        REQUIRE(subsets[0].start() == 0);
        REQUIRE(subsets[0].stop() == rows);
        REQUIRE(subsets[0].stride() == 1);
        REQUIRE(subsets[1].start() == 9);
        REQUIRE(subsets[1].stop() == 10);
        REQUIRE(subsets[1].stride() == 1);

        array.slice<float>(subsets);

        // Result: 1D array with 10 elements (all rows, column 9)
        REQUIRE(array.rank() == 1);
        REQUIRE(array.size() == rows);
        REQUIRE(array.shape() == std::vector<size_t>{rows});

        auto result = array.to_vector<float>();
        std::vector<float> expected = {9.0, 109.0, 209.0, 309.0, 409.0, 509.0, 609.0, 709.0, 809.0, 909.0};
        REQUIRE(result == expected);
    }

    SECTION("Select single column [:][0] - first column")
    {
        TypedDataArray array{data, {rows, cols}};
        auto subsets = parse_slices("[:][0]", array.shape());

        array.slice<float>(subsets);

        // Result: 1D array with 10 elements (all rows, column 0)
        REQUIRE(array.rank() == 1);
        REQUIRE(array.size() == rows);
        REQUIRE(array.shape() == std::vector<size_t>{rows});

        auto result = array.to_vector<float>();
        std::vector<float> expected = {0.0, 100.0, 200.0, 300.0, 400.0, 500.0, 600.0, 700.0, 800.0, 900.0};
        REQUIRE(result == expected);
    }

    SECTION("Select single column [:][14] - last column")
    {
        TypedDataArray array{data, {rows, cols}};
        auto subsets = parse_slices("[:][14]", array.shape());

        array.slice<float>(subsets);

        // Result: 1D array with 10 elements (all rows, column 14)
        REQUIRE(array.rank() == 1);
        REQUIRE(array.size() == rows);
        REQUIRE(array.shape() == std::vector<size_t>{rows});

        auto result = array.to_vector<float>();
        std::vector<float> expected = {14.0, 114.0, 214.0, 314.0, 414.0, 514.0, 614.0, 714.0, 814.0, 914.0};
        REQUIRE(result == expected);
    }

    SECTION("Select single row [5][:]")
    {
        TypedDataArray array{data, {rows, cols}};
        auto subsets = parse_slices("[5][:]", array.shape());

        array.slice<float>(subsets);

        // Result: 1D array with 10 elements (all columns, row 5)
        REQUIRE(array.rank() == 1);
        REQUIRE(array.size() == cols);
        REQUIRE(array.shape() == std::vector<size_t>{cols});

        auto result = array.to_vector<float>();
        std::vector<float> expected = {500.0, 501.0, 502.0, 503.0, 504.0, 505.0, 506.0, 507.0, 508.0, 509.0, 510.0, 511.0, 512.0, 513.0, 514.0};
        REQUIRE(result == expected);
    }

    SECTION("Select single element [7][9] - should reduce to 0D")
    {
        TypedDataArray array{data, {rows, cols}};
        auto subsets = parse_slices("[7][9]", array.shape());

        array.slice<float>(subsets);

        // Result: single value - row 7, column 9 - 709
        REQUIRE(array.rank() == 0);
        REQUIRE(array.size() == 1);
        REQUIRE(array.shape() == std::vector<size_t>{});

        REQUIRE(*reinterpret_cast<float*>(array.buffer()) == 709.0);
    }

    SECTION("Select column range with stride [:][2:12:3]")
    {
        TypedDataArray array{data, {rows, cols}};
        auto subsets = parse_slices("[:][2:12:3]", array.shape());

        array.slice<float>(subsets);

        // Result: 10 rows x 4 columns (indices 2, 5, 8, 11)
        REQUIRE(array.rank() == 2);
        REQUIRE(array.size() == rows * 4);
        REQUIRE(array.shape() == std::vector<size_t>{rows, 4});

        auto result = array.to_vector<float>();
        std::vector<float> expected = {
            2.0, 5.0, 8.0, 11.0,
            102.0, 105.0, 108.0, 111.0,
            202.0, 205.0, 208.0, 211.0,
            302.0, 305.0, 308.0, 311.0,
            402.0, 405.0, 408.0, 411.0,
            502.0, 505.0, 508.0, 511.0,
            602.0, 605.0, 608.0, 611.0,
            702.0, 705.0, 708.0, 711.0,
            802.0, 805.0, 808.0, 811.0,
            902.0, 905.0, 908.0, 911.0
        };
        REQUIRE(result == expected);
    }

    SECTION("Select every other row in order and also index [0:9:2][5]")
    {
        TypedDataArray array{data, {rows, cols}};
        auto subsets = parse_slices("[0:9:2][5]", array.shape());

        array.slice<float>(subsets);

        // Result: 1D array with 5 elements (rows 0, 2, 4, 6, 8, column 5)
        REQUIRE(array.rank() == 1);
        REQUIRE(array.size() == 5);
        REQUIRE(array.shape() == std::vector<size_t>{5});

        auto result = array.to_vector<float>();
        std::vector<float> expected = {5.0, 205.0, 405.0, 605.0, 805.0};
        REQUIRE(result == expected);
    }

    SECTION("Select column range with negative stride [:][12:2:-2]")
    {
        TypedDataArray array{data, {rows, cols}};
        auto subsets = parse_slices("[:][12:2:-2]", array.shape());

        array.slice<float>(subsets);

        // Result: 10 rows x 5 columns (indices 12, 10, 8, 6, 4)
        REQUIRE(array.rank() == 2);
        REQUIRE(array.size() == rows * 5);
        REQUIRE(array.shape() == std::vector<size_t>{rows, 5});

        auto result = array.to_vector<float>();
        std::vector<float> expected = {
            12.0, 10.0, 8.0, 6.0, 4.0,
            112.0, 110.0, 108.0, 106.0, 104.0,
            212.0, 210.0, 208.0, 206.0, 204.0,
            312.0, 310.0, 308.0, 306.0, 304.0,
            412.0, 410.0, 408.0, 406.0, 404.0,
            512.0, 510.0, 508.0, 506.0, 504.0,
            612.0, 610.0, 608.0, 606.0, 604.0,
            712.0, 710.0, 708.0, 706.0, 704.0,
            812.0, 810.0, 808.0, 806.0, 804.0,
            912.0, 910.0, 908.0, 906.0, 904.0
        };
        REQUIRE(result == expected);
    }

     SECTION("Select column range with -1 stride [:][14:0:-1]")
     {
         TypedDataArray array{data, {rows, cols}};
         auto subsets = parse_slices("[:][14:0:-1]", array.shape());

         array.slice<float>(subsets);

         // Result: 10 rows x 14 columns (indices 14 down to 1)
         REQUIRE(array.rank() == 2);
         REQUIRE(array.size() == rows * 14);
         REQUIRE(array.shape() == std::vector<size_t>{rows, 14});

         auto result = array.to_vector<float>();
         std::vector<float> expected = {
             14.0, 13.0, 12.0, 11.0, 10.0, 9.0, 8.0, 7.0, 6.0, 5.0, 4.0, 3.0, 2.0, 1.0,
             114.0, 113.0, 112.0, 111.0, 110.0, 109.0, 108.0, 107.0, 106.0, 105.0, 104.0, 103.0, 102.0, 101.0,
             214.0, 213.0, 212.0, 211.0, 210.0, 209.0, 208.0, 207.0, 206.0, 205.0, 204.0, 203.0, 202.0, 201.0,
             314.0, 313.0, 312.0, 311.0, 310.0, 309.0, 308.0, 307.0, 306.0, 305.0, 304.0, 303.0, 302.0, 301.0,
             414.0, 413.0, 412.0, 411.0, 410.0, 409.0, 408.0, 407.0, 406.0, 405.0, 404.0, 403.0, 402.0, 401.0,
             514.0, 513.0, 512.0, 511.0, 510.0, 509.0, 508.0, 507.0, 506.0, 505.0, 504.0, 503.0, 502.0, 501.0,
             614.0, 613.0, 612.0, 611.0, 610.0, 609.0, 608.0, 607.0, 606.0, 605.0, 604.0, 603.0, 602.0, 601.0,
             714.0, 713.0, 712.0, 711.0, 710.0, 709.0, 708.0, 707.0, 706.0, 705.0, 704.0, 703.0, 702.0, 701.0,
             814.0, 813.0, 812.0, 811.0, 810.0, 809.0, 808.0, 807.0, 806.0, 805.0, 804.0, 803.0, 802.0, 801.0,
             914.0, 913.0, 912.0, 911.0, 910.0, 909.0, 908.0, 907.0, 906.0, 905.0, 904.0, 903.0, 902.0, 901.0
         };
         REQUIRE(result == expected);
     }

     SECTION("Select row range with -1 stride [9:0:-1][:]")
     {
         TypedDataArray array{data, {rows, cols}};
         auto subsets = parse_slices("[9:0:-1][:]", array.shape());

         array.slice<float>(subsets);

         // Result: 9 rows x 15 columns (rows 9 down to 1)
         REQUIRE(array.rank() == 2);
         REQUIRE(array.size() == 9 * cols);
         REQUIRE(array.shape() == std::vector<size_t>{9, cols});

         auto result = array.to_vector<float>();
         std::vector<float> expected = {
             900.0, 901.0, 902.0, 903.0, 904.0, 905.0, 906.0, 907.0, 908.0, 909.0, 910.0, 911.0, 912.0, 913.0, 914.0,
             800.0, 801.0, 802.0, 803.0, 804.0, 805.0, 806.0, 807.0, 808.0, 809.0, 810.0, 811.0, 812.0, 813.0, 814.0,
             700.0, 701.0, 702.0, 703.0, 704.0, 705.0, 706.0, 707.0, 708.0, 709.0, 710.0, 711.0, 712.0, 713.0, 714.0,
             600.0, 601.0, 602.0, 603.0, 604.0, 605.0, 606.0, 607.0, 608.0, 609.0, 610.0, 611.0, 612.0, 613.0, 614.0,
             500.0, 501.0, 502.0, 503.0, 504.0, 505.0, 506.0, 507.0, 508.0, 509.0, 510.0, 511.0, 512.0, 513.0, 514.0,
             400.0, 401.0, 402.0, 403.0, 404.0, 405.0, 406.0, 407.0, 408.0, 409.0, 410.0, 411.0, 412.0, 413.0, 414.0,
             300.0, 301.0, 302.0, 303.0, 304.0, 305.0, 306.0, 307.0, 308.0, 309.0, 310.0, 311.0, 312.0, 313.0, 314.0,
             200.0, 201.0, 202.0, 203.0, 204.0, 205.0, 206.0, 207.0, 208.0, 209.0, 210.0, 211.0, 212.0, 213.0, 214.0,
             100.0, 101.0, 102.0, 103.0, 104.0, 105.0, 106.0, 107.0, 108.0, 109.0, 110.0, 111.0, 112.0, 113.0, 114.0
         };
         REQUIRE(result == expected);
     }


     SECTION("Select every other row in reverse and also index [9:0:-2][5]")
     {
         TypedDataArray array{data, {rows, cols}};
         auto subsets = parse_slices("[9:0:-2][5]", array.shape());

         array.slice<float>(subsets);

         // Result: 1D array with 5 elements (rows 9, 7, 5, 3, 1, column 5)
         REQUIRE(array.rank() == 1);
         REQUIRE(array.size() == 5);
         REQUIRE(array.shape() == std::vector<size_t>{5});

         auto result = array.to_vector<float>();
         std::vector<float> expected = {905.0, 705.0, 505.0, 305.0, 105.0};
         REQUIRE(result == expected);
     }
}

TEST_CASE("Subset 3D to 2D array", "[subset]")
{
    constexpr size_t dim1 = 5;
    constexpr size_t dim2 = 6;
    constexpr size_t dim3 = 8;

    // 5x6x8 row-major
    // Format: [i][j][k] = i*10000 + j*100 + k
    std::vector<int> data = {
        0, 1, 2, 3, 4, 5, 6, 7,
        100, 101, 102, 103, 104, 105, 106, 107,
        200, 201, 202, 203, 204, 205, 206, 207,
        300, 301, 302, 303, 304, 305, 306, 307,
        400, 401, 402, 403, 404, 405, 406, 407,
        500, 501, 502, 503, 504, 505, 506, 507,

        10000, 10001, 10002, 10003, 10004, 10005, 10006, 10007,
        10100, 10101, 10102, 10103, 10104, 10105, 10106, 10107,
        10200, 10201, 10202, 10203, 10204, 10205, 10206, 10207,
        10300, 10301, 10302, 10303, 10304, 10305, 10306, 10307,
        10400, 10401, 10402, 10403, 10404, 10405, 10406, 10407,
        10500, 10501, 10502, 10503, 10504, 10505, 10506, 10507,

        20000, 20001, 20002, 20003, 20004, 20005, 20006, 20007,
        20100, 20101, 20102, 20103, 20104, 20105, 20106, 20107,
        20200, 20201, 20202, 20203, 20204, 20205, 20206, 20207,
        20300, 20301, 20302, 20303, 20304, 20305, 20306, 20307,
        20400, 20401, 20402, 20403, 20404, 20405, 20406, 20407,
        20500, 20501, 20502, 20503, 20504, 20505, 20506, 20507,

        30000, 30001, 30002, 30003, 30004, 30005, 30006, 30007,
        30100, 30101, 30102, 30103, 30104, 30105, 30106, 30107,
        30200, 30201, 30202, 30203, 30204, 30205, 30206, 30207,
        30300, 30301, 30302, 30303, 30304, 30305, 30306, 30307,
        30400, 30401, 30402, 30403, 30404, 30405, 30406, 30407,
        30500, 30501, 30502, 30503, 30504, 30505, 30506, 30507,

        40000, 40001, 40002, 40003, 40004, 40005, 40006, 40007,
        40100, 40101, 40102, 40103, 40104, 40105, 40106, 40107,
        40200, 40201, 40202, 40203, 40204, 40205, 40206, 40207,
        40300, 40301, 40302, 40303, 40304, 40305, 40306, 40307,
        40400, 40401, 40402, 40403, 40404, 40405, 40406, 40407,
        40500, 40501, 40502, 40503, 40504, 40505, 40506, 40507
    };

    SECTION("Select single third dimension [:][:][4]")
    {
        TypedDataArray array{data, {dim1, dim2, dim3}};

        auto subsets = parse_slices("[:][:][4]", array.shape());

        REQUIRE(subsets.size() == 3);
        REQUIRE(subsets[0].start() == 0);
        REQUIRE(subsets[0].stop() == dim1);
        REQUIRE(subsets[0].stride() == 1);
        REQUIRE(subsets[1].start() == 0);
        REQUIRE(subsets[1].stop() == dim2);
        REQUIRE(subsets[1].stride() == 1);
        REQUIRE(subsets[2].start() == 4);
        REQUIRE(subsets[2].stop() == 5);
        REQUIRE(subsets[2].stride() == 1);

        array.slice<int>(subsets);

        // Result: 2D array with 5(dim1) x 6(dim2) elements
        REQUIRE(array.rank() == 2);
        REQUIRE(array.size() == dim1 * dim2);
        REQUIRE(array.shape() == std::vector<size_t>{dim1, dim2});

        auto result = array.to_vector<int>();
        std::vector<int> expected = {
            4, 104, 204, 304, 404, 504,
            10004, 10104, 10204, 10304, 10404, 10504,
            20004, 20104, 20204, 20304, 20404, 20504,
            30004, 30104, 30204, 30304, 30404, 30504,
            40004, 40104, 40204, 40304, 40404, 40504
        };
        REQUIRE(result == expected);
    }

    SECTION("Select first element of third dimension [:][:][0]")
    {
        TypedDataArray array{data, {dim1, dim2, dim3}};
        auto subsets = parse_slices("[:][:][0]", array.shape());

        array.slice<int>(subsets);

        REQUIRE(array.rank() == 2);
        REQUIRE(array.size() == dim1 * dim2);
        REQUIRE(array.shape() == std::vector<size_t>{dim1, dim2});

        auto result = array.to_vector<int>();
        std::vector<int> expected = {
            0, 100, 200, 300, 400, 500,
            10000, 10100, 10200, 10300, 10400, 10500,
            20000, 20100, 20200, 20300, 20400, 20500,
            30000, 30100, 30200, 30300, 30400, 30500,
            40000, 40100, 40200, 40300, 40400, 40500
        };
        REQUIRE(result == expected);
    }

    SECTION("Select last element of third dimension [:][:][7]")
    {
        TypedDataArray array{data, {dim1, dim2, dim3}};
        auto subsets = parse_slices("[:][:][7]", array.shape());

        array.slice<int>(subsets);

        REQUIRE(array.rank() == 2);
        REQUIRE(array.size() == dim1 * dim2);
        REQUIRE(array.shape() == std::vector<size_t>{dim1, dim2});

        auto result = array.to_vector<int>();
        std::vector<int> expected = {
            7, 107, 207, 307, 407, 507,
            10007, 10107, 10207, 10307, 10407, 10507,
            20007, 20107, 20207, 20307, 20407, 20507,
            30007, 30107, 30207, 30307, 30407, 30507,
            40007, 40107, 40207, 40307, 40407, 40507
        };
        REQUIRE(result == expected);
    }

    SECTION("Select single first dimension [2][:][:]")
    {
        TypedDataArray array{data, {dim1, dim2, dim3}};
        auto subsets = parse_slices("[2][:][:]", array.shape());

        array.slice<int>(subsets);

        REQUIRE(array.rank() == 2);
        REQUIRE(array.size() == dim2 * dim3);
        REQUIRE(array.shape() == std::vector<size_t>{dim2, dim3});

        auto result = array.to_vector<int>();
        std::vector<int> expected = {
            20000, 20001, 20002, 20003, 20004, 20005, 20006, 20007,
            20100, 20101, 20102, 20103, 20104, 20105, 20106, 20107,
            20200, 20201, 20202, 20203, 20204, 20205, 20206, 20207,
            20300, 20301, 20302, 20303, 20304, 20305, 20306, 20307,
            20400, 20401, 20402, 20403, 20404, 20405, 20406, 20407,
            20500, 20501, 20502, 20503, 20504, 20505, 20506, 20507
        };
        REQUIRE(result == expected);
    }

    SECTION("Select single second dimension [:][3][:]")
    {
        TypedDataArray array{data, {dim1, dim2, dim3}};
        auto subsets = parse_slices("[:][3][:]", array.shape());

        array.slice<int>(subsets);

        REQUIRE(array.rank() == 2);
        REQUIRE(array.size() == dim1 * dim3);
        REQUIRE(array.shape() == std::vector<size_t>{dim1, dim3});

        auto result = array.to_vector<int>();
        std::vector<int> expected = {
            300, 301, 302, 303, 304, 305, 306, 307,
            10300, 10301, 10302, 10303, 10304, 10305, 10306, 10307,
            20300, 20301, 20302, 20303, 20304, 20305, 20306, 20307,
            30300, 30301, 30302, 30303, 30304, 30305, 30306, 30307,
            40300, 40301, 40302, 40303, 40304, 40305, 40306, 40307
        };
        REQUIRE(result == expected);
    }

    SECTION("Select single element from all three dimensions [2][3][4] - should reduce to 0D")
    {
        TypedDataArray array{data, {dim1, dim2, dim3}};
        auto subsets = parse_slices("[2][3][4]", array.shape());

        array.slice<int>(subsets);

        REQUIRE(array.rank() == 0);
        REQUIRE(array.size() == 1);
        REQUIRE(array.shape() == std::vector<size_t>{});

        // Result: 20304
        REQUIRE(*reinterpret_cast<int*>(array.buffer()) == 20304);
    }

    SECTION("Select range in third dimension [:][:][2:6]")
    {
        TypedDataArray array{data, {dim1, dim2, dim3}};
        auto subsets = parse_slices("[:][:][2:6]", array.shape());

        array.slice<int>(subsets);

        REQUIRE(array.rank() == 3);
        REQUIRE(array.size() == dim1 * dim2 * 4);
        REQUIRE(array.shape() == std::vector<size_t>{dim1, dim2, 4});

        auto result = array.to_vector<int>();
        std::vector<int> expected = {
            2, 3, 4, 5,
            102, 103, 104, 105,
            202, 203, 204, 205,
            302, 303, 304, 305,
            402, 403, 404, 405,
            502, 503, 504, 505,

            10002, 10003, 10004, 10005,
            10102, 10103, 10104, 10105,
            10202, 10203, 10204, 10205,
            10302, 10303, 10304, 10305,
            10402, 10403, 10404, 10405,
            10502, 10503, 10504, 10505,

            20002, 20003, 20004, 20005,
            20102, 20103, 20104, 20105,
            20202, 20203, 20204, 20205,
            20302, 20303, 20304, 20305,
            20402, 20403, 20404, 20405,
            20502, 20503, 20504, 20505,

            30002, 30003, 30004, 30005,
            30102, 30103, 30104, 30105,
            30202, 30203, 30204, 30205,
            30302, 30303, 30304, 30305,
            30402, 30403, 30404, 30405,
            30502, 30503, 30504, 30505,

            40002, 40003, 40004, 40005,
            40102, 40103, 40104, 40105,
            40202, 40203, 40204, 40205,
            40302, 40303, 40304, 40305,
            40402, 40403, 40404, 40405,
            40502, 40503, 40504, 40505
        };
        REQUIRE(result == expected);
    }

    SECTION("Select with stride in third dimension [:][:][1:7:2]")
    {
        TypedDataArray array{data, {dim1, dim2, dim3}};
        auto subsets = parse_slices("[:][:][1:7:2]", array.shape());

        array.slice<int>(subsets);

        REQUIRE(array.rank() == 3);
        REQUIRE(array.size() == dim1 * dim2 * 3);
        REQUIRE(array.shape() == std::vector<size_t>{dim1, dim2, 3});

        auto result = array.to_vector<int>();
        std::vector<int> expected = {
            1, 3, 5,
            101, 103, 105,
            201, 203, 205,
            301, 303, 305,
            401, 403, 405,
            501, 503, 505,

            10001, 10003, 10005,
            10101, 10103, 10105,
            10201, 10203, 10205,
            10301, 10303, 10305,
            10401, 10403, 10405,
            10501, 10503, 10505,

            20001, 20003, 20005,
            20101, 20103, 20105,
            20201, 20203, 20205,
            20301, 20303, 20305,
            20401, 20403, 20405,
            20501, 20503, 20505,

            30001, 30003, 30005,
            30101, 30103, 30105,
            30201, 30203, 30205,
            30301, 30303, 30305,
            30401, 30403, 30405,
            30501, 30503, 30505,

            40001, 40003, 40005,
            40101, 40103, 40105,
            40201, 40203, 40205,
            40301, 40303, 40305,
            40401, 40403, 40405,
            40501, 40503, 40505
        };
        REQUIRE(result == expected);
    }

    SECTION("Select with -1 stride in third dimension [:][:][7:0:-1]")
    {
        TypedDataArray array{data, {dim1, dim2, dim3}};
        auto subsets = parse_slices("[:][:][7:0:-1]", array.shape());

        array.slice<int>(subsets);

        REQUIRE(array.rank() == 3);
        REQUIRE(array.size() == dim1 * dim2 * 7);
        REQUIRE(array.shape() == std::vector<size_t>{dim1, dim2, 7});

        auto result = array.to_vector<int>();
        std::vector<int> expected = {
            7, 6, 5, 4, 3, 2, 1,
            107, 106, 105, 104, 103, 102, 101,
            207, 206, 205, 204, 203, 202, 201,
            307, 306, 305, 304, 303, 302, 301,
            407, 406, 405, 404, 403, 402, 401,
            507, 506, 505, 504, 503, 502, 501,

            10007, 10006, 10005, 10004, 10003, 10002, 10001,
            10107, 10106, 10105, 10104, 10103, 10102, 10101,
            10207, 10206, 10205, 10204, 10203, 10202, 10201,
            10307, 10306, 10305, 10304, 10303, 10302, 10301,
            10407, 10406, 10405, 10404, 10403, 10402, 10401,
            10507, 10506, 10505, 10504, 10503, 10502, 10501,

            20007, 20006, 20005, 20004, 20003, 20002, 20001,
            20107, 20106, 20105, 20104, 20103, 20102, 20101,
            20207, 20206, 20205, 20204, 20203, 20202, 20201,
            20307, 20306, 20305, 20304, 20303, 20302, 20301,
            20407, 20406, 20405, 20404, 20403, 20402, 20401,
            20507, 20506, 20505, 20504, 20503, 20502, 20501,

            30007, 30006, 30005, 30004, 30003, 30002, 30001,
            30107, 30106, 30105, 30104, 30103, 30102, 30101,
            30207, 30206, 30205, 30204, 30203, 30202, 30201,
            30307, 30306, 30305, 30304, 30303, 30302, 30301,
            30407, 30406, 30405, 30404, 30403, 30402, 30401,
            30507, 30506, 30505, 30504, 30503, 30502, 30501,

            40007, 40006, 40005, 40004, 40003, 40002, 40001,
            40107, 40106, 40105, 40104, 40103, 40102, 40101,
            40207, 40206, 40205, 40204, 40203, 40202, 40201,
            40307, 40306, 40305, 40304, 40303, 40302, 40301,
            40407, 40406, 40405, 40404, 40403, 40402, 40401,
            40507, 40506, 40505, 40504, 40503, 40502, 40501
        };
        REQUIRE(result == expected);
    }

    SECTION("Select with negative stride in first dimension [4:0:-1][:][3]")
    {
        TypedDataArray array{data, {dim1, dim2, dim3}};
        auto subsets = parse_slices("[4:0:-1][:][3]", array.shape());

        array.slice<int>(subsets);

        REQUIRE(array.rank() == 2);
        REQUIRE(array.size() == 4 * dim2);
        REQUIRE(array.shape() == std::vector<size_t>{4, dim2});

        auto result = array.to_vector<int>();
        std::vector<int> expected = {
            40003, 40103, 40203, 40303, 40403, 40503,
            30003, 30103, 30203, 30303, 30403, 30503,
            20003, 20103, 20203, 20303, 20403, 20503,
            10003, 10103, 10203, 10303, 10403, 10503
        };
        REQUIRE(result == expected);
    }

    SECTION("Select with negative stride in second dimension [:][5:0:-2][:]")
    {
        TypedDataArray array{data, {dim1, dim2, dim3}};
        auto subsets = parse_slices("[:][5:0:-2][:]", array.shape());

        array.slice<int>(subsets);

        REQUIRE(array.rank() == 3);
        REQUIRE(array.size() == dim1 * 3 * dim3);
        REQUIRE(array.shape() == std::vector<size_t>{dim1, 3, dim3});

        auto result = array.to_vector<int>();
        std::vector<int> expected = {
            500, 501, 502, 503, 504, 505, 506, 507,
            300, 301, 302, 303, 304, 305, 306, 307,
            100, 101, 102, 103, 104, 105, 106, 107,

            10500, 10501, 10502, 10503, 10504, 10505, 10506, 10507,
            10300, 10301, 10302, 10303, 10304, 10305, 10306, 10307,
            10100, 10101, 10102, 10103, 10104, 10105, 10106, 10107,

            20500, 20501, 20502, 20503, 20504, 20505, 20506, 20507,
            20300, 20301, 20302, 20303, 20304, 20305, 20306, 20307,
            20100, 20101, 20102, 20103, 20104, 20105, 20106, 20107,

            30500, 30501, 30502, 30503, 30504, 30505, 30506, 30507,
            30300, 30301, 30302, 30303, 30304, 30305, 30306, 30307,
            30100, 30101, 30102, 30103, 30104, 30105, 30106, 30107,

            40500, 40501, 40502, 40503, 40504, 40505, 40506, 40507,
            40300, 40301, 40302, 40303, 40304, 40305, 40306, 40307,
            40100, 40101, 40102, 40103, 40104, 40105, 40106, 40107
        };
        REQUIRE(result == expected);
    }
}

TEST_CASE("Subset with update_array function", "[subset]")
{
    SECTION("2D to 1D with update_array")
    {
        constexpr size_t rows = 8;
        constexpr size_t cols = 10;

        std::vector<double> data = {
            0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0,
            10.0, 11.0, 12.0, 13.0, 14.0, 15.0, 16.0, 17.0, 18.0, 19.0,
            20.0, 21.0, 22.0, 23.0, 24.0, 25.0, 26.0, 27.0, 28.0, 29.0,
            30.0, 31.0, 32.0, 33.0, 34.0, 35.0, 36.0, 37.0, 38.0, 39.0,
            40.0, 41.0, 42.0, 43.0, 44.0, 45.0, 46.0, 47.0, 48.0, 49.0,
            50.0, 51.0, 52.0, 53.0, 54.0, 55.0, 56.0, 57.0, 58.0, 59.0,
            60.0, 61.0, 62.0, 63.0, 64.0, 65.0, 66.0, 67.0, 68.0, 69.0,
            70.0, 71.0, 72.0, 73.0, 74.0, 75.0, 76.0, 77.0, 78.0, 79.0
        };

        TypedDataArray array{data, {rows, cols}};

        update_array(array, "[:][5]", std::nullopt, std::nullopt);

        REQUIRE(array.rank() == 1);
        REQUIRE(array.size() == rows);
        REQUIRE(array.shape() == std::vector<size_t>{rows});

        auto result = array.to_vector<double>();
        std::vector<double> expected = {5.0, 15.0, 25.0, 35.0, 45.0, 55.0, 65.0, 75.0};
        REQUIRE(result == expected);
    }

    SECTION("3D to 2D with update_array")
    {
        constexpr size_t dim1 = 4;
        constexpr size_t dim2 = 5;
        constexpr size_t dim3 = 6;

        std::vector<double> data = {
            0.0, 1.0, 2.0, 3.0, 4.0, 5.0,
            6.0, 7.0, 8.0, 9.0, 10.0, 11.0,
            12.0, 13.0, 14.0, 15.0, 16.0, 17.0,
            18.0, 19.0, 20.0, 21.0, 22.0, 23.0,
            24.0, 25.0, 26.0, 27.0, 28.0, 29.0,

            30.0, 31.0, 32.0, 33.0, 34.0, 35.0,
            36.0, 37.0, 38.0, 39.0, 40.0, 41.0,
            42.0, 43.0, 44.0, 45.0, 46.0, 47.0,
            48.0, 49.0, 50.0, 51.0, 52.0, 53.0,
            54.0, 55.0, 56.0, 57.0, 58.0, 59.0,

            60.0, 61.0, 62.0, 63.0, 64.0, 65.0,
            66.0, 67.0, 68.0, 69.0, 70.0, 71.0,
            72.0, 73.0, 74.0, 75.0, 76.0, 77.0,
            78.0, 79.0, 80.0, 81.0, 82.0, 83.0,
            84.0, 85.0, 86.0, 87.0, 88.0, 89.0,

            90.0, 91.0, 92.0, 93.0, 94.0, 95.0,
            96.0, 97.0, 98.0, 99.0, 100.0, 101.0,
            102.0, 103.0, 104.0, 105.0, 106.0, 107.0,
            108.0, 109.0, 110.0, 111.0, 112.0, 113.0,
            114.0, 115.0, 116.0, 117.0, 118.0, 119.0
        };

        TypedDataArray array{data, {dim1, dim2, dim3}};

        // Use update_array to slice [:][:][3]
        update_array(array, "[:][:][3]", std::nullopt, std::nullopt);

        REQUIRE(array.rank() == 2);
        REQUIRE(array.size() == dim1 * dim2);
        REQUIRE(array.shape() == std::vector<size_t>{dim1, dim2});

        auto result = array.to_vector<double>();
        std::vector<double> expected = {
            3.0, 9.0, 15.0, 21.0, 27.0,
            33.0, 39.0, 45.0, 51.0, 57.0,
            63.0, 69.0, 75.0, 81.0, 87.0,
            93.0, 99.0, 105.0, 111.0, 117.0
        };
        REQUIRE(result == expected);
    }

    SECTION("2D to 1D with scale and offset")
    {
        constexpr size_t rows = 5;
        constexpr size_t cols = 7;

        std::vector<float> data = {
            0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0,
            7.0, 8.0, 9.0, 10.0, 11.0, 12.0, 13.0,
            14.0, 15.0, 16.0, 17.0, 18.0, 19.0, 20.0,
            21.0, 22.0, 23.0, 24.0, 25.0, 26.0, 27.0,
            28.0, 29.0, 30.0, 31.0, 32.0, 33.0, 34.0
        };

        TypedDataArray array{data, {rows, cols}};

        // Use update_array to slice [:][3] and apply scale=2.0, offset=10.0
        update_array(array, "[:][3]", 2.0, 10.0);

        REQUIRE(array.rank() == 1);
        REQUIRE(array.size() == rows);

        // After scale and offset: (3*2+10=16), (10*2+10=30), (17*2+10=44), (24*2+10=58), (31*2+10=72)
        auto result = array.to_vector<float>();
        std::vector<float> expected = {16.0, 30.0, 44.0, 58.0, 72.0};
        REQUIRE(result == expected);
    }


    SECTION("3D to 2D with scale and offset")
    {
        constexpr size_t dim1 = 3;
        constexpr size_t dim2 = 4;
        constexpr size_t dim3 = 5;

        std::vector<float> data = {
            0.0, 10.0, 20.0, 30.0, 40.0,
            50.0, 60.0, 70.0, 80.0, 90.0,
            100.0, 110.0, 120.0, 130.0, 140.0,
            150.0, 160.0, 170.0, 180.0, 190.0,

            200.0, 210.0, 220.0, 230.0, 240.0,
            250.0, 260.0, 270.0, 280.0, 290.0,
            300.0, 310.0, 320.0, 330.0, 340.0,
            350.0, 360.0, 370.0, 380.0, 390.0,

            400.0, 410.0, 420.0, 430.0, 440.0,
            450.0, 460.0, 470.0, 480.0, 490.0,
            500.0, 510.0, 520.0, 530.0, 540.0,
            550.0, 560.0, 570.0, 580.0, 590.0
        };

        TypedDataArray array{data, {dim1, dim2, dim3}};

        // Use update_array to slice [:][:][2] and apply scale=0.5, offset=100.0
        update_array(array, "[:][:][2]", 0.5f, 100.0);

        REQUIRE(array.rank() == 2);
        REQUIRE(array.size() == dim1 * dim2);

        // k=2 values: 20, 70, 120, 170, 220, 270, 320, 370, 420, 470, 520, 570
        // After scale and offset: value*0.5+100.0
        auto result = array.to_vector<float>();
        std::vector<float> expected = {
            110.0, 135.0, 160.0, 185.0,
            210.0, 235.0, 260.0, 285.0,
            310.0, 335.0, 360.0, 385.0
        };
        REQUIRE(result == expected);
    }
}
