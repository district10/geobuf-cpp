#include "geobuf/geobuf.hpp"
#include "geobuf/version.h"

#define DBG_MACRO_NO_WARNING
#include <dbg.h>

// https://github.com/onqtam/doctest
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

std::string FIXTURES_DIR = PROJECT_SOURCE_DIR "/geobuf/test/fixtures";
std::vector<std::string> FIXTURES = {
    "issue55.json",          //
    "issue62.json",          //
    "issue90.json",          //
    "precision.json",        //
    "props.json",            //
    "single-multipoly.json", //
};

void roundtripTest(const std::string &path)
{
    // json -> pbf -> json
    // TODO
}

TEST_CASE("load configs")
{
    dbg(FIXTURES_DIR);
    dbg(FIXTURES);
}
