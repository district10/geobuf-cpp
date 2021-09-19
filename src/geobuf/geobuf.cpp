#include "geobuf/geobuf.hpp"

int answer() { return 42; }

namespace mapbox
{
namespace geobuf
{

Pbf Encoder::encode(const mapbox::geojson::geojson &geojson) {}

void Encoder::analyze(const mapbox::geojson::geojson &geojson) {}
void Encoder::analyzeMultiLine(
    const mapbox::geojson::multi_line_string &geojson)
{
}
void Encoder::analyzePoints(const mapbox::geojson::multi_point &points)
{
    for (auto &pt : points) {
        analyzePoint(pt);
    }
}
void Encoder::analyzePoint(const mapbox::geojson::point &point)
{
    // TODO
}
void Encoder::saveKey(const std::string &key)
{
    if (keys.find(key) != keys.end()) {
        return;
    }
    keys.emplace(key, keys.size());
}

void Encoder::writeFeatureCollection(
    const mapbox::geojson::feature_collection &geojson, Pbf &pbf)
{
}

void Encoder::writeFeature(const mapbox::geojson::feature &geojson, Pbf &pbf) {}
}
}