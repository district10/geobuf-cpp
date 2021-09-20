#include "geobuf/geobuf.hpp"
#include <iostream>

int main(int argc, char *argv[])
{
    auto encoder = mapbox::geobuf::Encoder();
    auto json = mapbox::geobuf::load_json(argv[1]);
    auto geojson = mapbox::geojson::convert(json);
    auto pbf = encoder.encode(geojson);
    std::cout << pbf;
    return 0;
}
