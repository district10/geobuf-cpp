#include "geobuf/geobuf.hpp"

#define DBG_MACRO_NO_WARNING
#include <dbg.h>

// https://github.com/onqtam/doctest
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

int factorial(int number)
{
    return number <= 1 ? number : factorial(number - 1) * number;
}

TEST_CASE("testing the factorial function")
{
    CHECK(dbg(factorial(1)) == 1);
    CHECK(dbg(factorial(2)) == 2);
    CHECK(dbg(factorial(3)) == 6);
    CHECK(dbg(factorial(10)) == 3628800);
}
