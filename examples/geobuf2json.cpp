#include "geobuf/geobuf.hpp"
#include <iostream>

int main(int argc, char *argv[])
{
    // usage:
    //      cat input.pbf | geobuf2json > output.json
    //      geobuf2json input.pbf > output.json
    //      geobuf2json input.pbf output.json
    auto decoder = mapbox::geobuf::Decoder();
    auto bytes = argc > 1 ? mapbox::geobuf::load_bytes(argv[1])
                          : mapbox::geobuf::load_bytes();
    auto geojson = decoder.decode(bytes);
    if (argc > 2) {
        return mapbox::geobuf::dump_bytes(argv[2],
                                          mapbox::geojson::stringify(geojson))
                   ? 0
                   : -1;
    } else {
        std::cout << mapbox::geojson::stringify(geojson);
    }
    return 0;
}
