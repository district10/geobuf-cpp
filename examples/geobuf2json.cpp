#include "geobuf/geobuf.hpp"
#include <iostream>

int main(int argc, char *argv[])
{
    auto decoder = mapbox::geobuf::Decoder();
    auto bytes = mapbox::geobuf::load_bytes(argv[1]);
    auto geojson = decoder.decode(bytes);
    std::cout << mapbox::geojson::stringify(geojson);
    return 0;
}
