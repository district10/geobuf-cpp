#include "geobuf/geobuf.hpp"
#include <iostream>

int main(int argc, char *argv[])
{
    // usage:
    //      cat input.json | lintjson > output.json
    //      lintjson input.json > output.json
    //      lintjson input.json output.json
    auto json = argc > 1 ? mapbox::geobuf::load_json(argv[1])
                         : mapbox::geobuf::load_json();
    return (argc > 2 ? mapbox::geobuf::dump_json(argv[2], json, true)
                     : mapbox::geobuf::dump_json(json, true))
               ? 0
               : -1;
}
