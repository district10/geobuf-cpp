#pragma once

#include <mapbox/geojson.hpp>
#include <protozero/pbf_builder.hpp>
#include <protozero/pbf_reader.hpp>

namespace mapbox
{
namespace geobuf
{
struct Encoder
{
    using Pbf = protozero::pbf_writer;
    Pbf encode(const mapbox::geojson::geojson &geojson);
    void analyze(const mapbox::geojson::geojson &geojson);
    void analyzeGeometry(const mapbox::geojson::geometry &geometry);
    void analyzeMultiLine(
        const mapbox::geojson::multi_line_string::container_type &lines);
    void
    analyzePoints(const mapbox::geojson::multi_point::container_type &points);
    void analyzePoint(const mapbox::geojson::point &point);
    void saveKey(const std::string &key);

    // Yeah, I know. In c++, we can use overloading...
    // Just make it identical to the JS implementation
    void
    writeFeatureCollection(const mapbox::geojson::feature_collection &geojson,
                           Pbf &pbf);
    void writeFeature(const mapbox::geojson::feature &geojson, Pbf &pbf);
    void writeGeometry(const mapbox::geojson::geometry &geojson, Pbf &pbf);
    void writeProps(const mapbox::feature::property_map &props, Pbf &pbf,
                    bool isCustom);
    void writeValue(const mapbox::feature::value &value, Pbf &pbf);
    void writePoint(const mapbox::geojson::point &point, Pbf &pbf);
    void writeLine(const mapbox::geojson::line_string &line, Pbf &pbf);
    void writeMultiLine(const mapbox::geojson::multi_line_string &lines,
                        Pbf &pbf);
    void writeMultiPolygon(const mapbox::geojson::multi_polygon &polygons,
                           Pbf &pbf);
    // function populateLine(coords, line, closed)
    // function isSpecialKey(key, type)
    static bool isSpecialKey(const std::string &key);

    uint32_t dim = 2;
    uint32_t e = 1;
    std::unordered_map<std::string, std::uint32_t> keys;
};

struct Decoder
{
    using Pbf = protozero::pbf_reader;
    mapbox::geojson::geojson encode(const Pbf &pbf);
    void readDataField(const Pbf &pbf);
    mapbox::geojson::feature_collection readFeatureCollection(const Pbf &pbf);
    mapbox::geojson::feature readFeature(const Pbf &pbf);
    void readGeometry(const Pbf &pbf);
    void readFeatureColectionField(const Pbf &pbf);
    void readFeatureField(const Pbf &pbf);
    void readGeometryField(const Pbf &pbf);
    void readCoords(const Pbf &pbf);
    void readValue(const Pbf &pbf);
    void readProps(const Pbf &pbf);
    void readPoint(const Pbf &pbf);
    void readLinePart(const Pbf &pbf);
    void readLine(const Pbf &pbf);
    void readMultiLine(const Pbf &pbf);
    void readMultiPolygon(const Pbf &pbf);
};

} // namespace goebuf
} // namespace mapbox
