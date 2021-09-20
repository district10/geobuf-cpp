#pragma once

#include <mapbox/geojson.hpp>
#include <mapbox/geojson/rapidjson.hpp>
#include <protozero/pbf_builder.hpp>
#include <protozero/pbf_reader.hpp>

namespace mapbox
{
namespace geobuf
{
using PointsType = mapbox::geojson::multi_point::container_type;
using LinesType = mapbox::geojson::multi_line_string::container_type;
using PolygonsType = mapbox::geojson::multi_polygon::container_type;

using RapidjsonValue = mapbox::geojson::rapidjson_value;

RapidjsonValue load_json(const std::string &path);
bool dump_json(const std::string &path, const RapidjsonValue &json,
               bool indent = false);

std::string load_bytes(const std::string &path);
bool dump_bytes(const std::string &path, const std::string &bytes);

RapidjsonValue parse(const std::string &json, bool raise_error = false);
std::string dump(const RapidjsonValue &json, bool indent = false);

struct Encoder
{
    using Pbf = protozero::pbf_writer;
    std::string encode(const mapbox::geojson::geojson &geojson);
    void analyze(const mapbox::geojson::geojson &geojson);
    void analyzeGeometry(const mapbox::geojson::geometry &geometry);
    void analyzeMultiLine(const LinesType &lines);
    void analyzePoints(const PointsType &points);
    void analyzePoint(const mapbox::geojson::point &point);
    void saveKey(const std::string &key);

    // Yeah, I know. In c++, we can use overloading...
    // Just make it identical to the JS implementation
    void
    writeFeatureCollection(const mapbox::geojson::feature_collection &geojson,
                           Pbf &pbf);
    void writeFeature(const mapbox::geojson::feature &geojson, Pbf &pbf);
    void writeGeometry(const mapbox::geojson::geometry &geojson, Pbf &pbf);
    // in mapbox geojson, there is no custom properties
    void writeProps(const mapbox::feature::property_map &props, Pbf &pbf);
    void writeValue(const mapbox::feature::value &value, Pbf &pbf);
    void writePoint(const mapbox::geojson::point &point, Pbf &pbf);
    void writeLine(const PointsType &line, Pbf &pbf);
    // implict close=false is JS, we don't do that
    void writeMultiLine(const LinesType &lines, Pbf &pbf, bool closed);
    void writeMultiPolygon(const PolygonsType &polygons, Pbf &pbf);
    std::vector<int64_t> populateLine(const PointsType &line, bool closed);
    void populateLine(std::vector<int64_t> &coords, //
                      const PointsType &line,       //
                      bool closed);

    uint32_t dim = 2;
    uint32_t e = 1;
    std::unordered_map<std::string, std::uint32_t> keys;
};

struct Decoder
{
    using Pbf = protozero::pbf_reader;
    static std::string to_printable(const std::string &pbf_bytes);

    mapbox::geojson::geojson decode(const Pbf &pbf);
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
