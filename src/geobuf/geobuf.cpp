#include "geobuf/geobuf.hpp"
#include "geobuf/pbf_decoder.cpp"

#include <mapbox/geojson_impl.hpp>
#include <mapbox/geojson_value_impl.hpp>

#include "rapidjson/error/en.h"
#include "rapidjson/filereadstream.h"
#include "rapidjson/filewritestream.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"
#include <fstream>

#include <protozero/pbf_builder.hpp>
#include <protozero/pbf_reader.hpp>

#define DBG_MACRO_NO_WARNING
#include "dbg.h"

constexpr const auto RJFLAGS = rapidjson::kParseDefaultFlags |
                               rapidjson::kParseCommentsFlag |
                               rapidjson::kParseTrailingCommasFlag;

constexpr uint32_t maxPrecision = 1e6;
constexpr uint32_t dimXY = 2;
constexpr uint32_t dimXYZ = 2;

using RapidjsonDocument = mapbox::geojson::rapidjson_document;

namespace mapbox
{
namespace geobuf
{
RapidjsonValue load_json(const std::string &path)
{
    FILE *fp = fopen(path.c_str(), "rb");
    if (!fp) {
        return {};
    }
    char readBuffer[65536];
    rapidjson::FileReadStream is(fp, readBuffer, sizeof(readBuffer));
    RapidjsonDocument d;
    d.ParseStream<RJFLAGS>(is);
    fclose(fp);
    return d.GetObject();
}
bool dump_json(const std::string &path, const RapidjsonValue &json, bool indent)
{
    using namespace rapidjson;
    FILE *fp = fopen(path.c_str(), "wb");
    if (!fp) {
        return false;
    }
    char writeBuffer[65536];
    FileWriteStream os(fp, writeBuffer, sizeof(writeBuffer));
    if (indent) {
        PrettyWriter<FileWriteStream> writer(os);
        json.Accept(writer);
    } else {
        Writer<FileWriteStream> writer(os);
        json.Accept(writer);
    }
    fclose(fp);
    return true;
}

std::string load_bytes(const std::string &path)
{
    std::ifstream t(path.c_str());
    std::stringstream buffer;
    buffer << t.rdbuf();
    return buffer.str();
}
bool dump_bytes(const std::string &path, const std::string &bytes)
{
    std::ofstream outfile(path, std::ofstream::binary | std::ios::out);
    if (!outfile) {
        return false;
    }
    outfile.write(bytes.data(), bytes.size());
    return true;
}

RapidjsonValue parse(const std::string &json, bool raise_error)
{
    RapidjsonDocument d;
    rapidjson::StringStream ss(json.c_str());
    d.ParseStream<RJFLAGS>(ss);
    if (d.HasParseError()) {
        if (raise_error) {
            throw std::invalid_argument(
                "invalid json, offset: " + std::to_string(d.GetErrorOffset()) +
                ", error: " + rapidjson::GetParseError_En(d.GetParseError()));
        } else {
            return RapidjsonValue{};
        }
    }
    return d.GetObject();
}

std::string dump(const RapidjsonValue &json, bool indent)
{
    rapidjson::StringBuffer buffer;
    if (indent) {
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
        json.Accept(writer);
    } else {
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        json.Accept(writer);
    }
    return buffer.GetString();
}

std::string Encoder::encode(const mapbox::geojson::geojson &geojson)
{
    assert(keys.empty());
    analyze(geojson);

    std::vector<std::pair<const std::string *, uint32_t>> keys_vec;
    keys_vec.reserve(keys.size());
    for (auto &pair : keys) {
        keys_vec.emplace_back(&pair.first, pair.second);
    }
    std::sort(keys_vec.begin(), keys_vec.end(),
              [](const auto &kv1, const auto &kv2) {
                  return kv1.second < kv2.second;
              });

    std::string data;
    Encoder::Pbf pbf{data};
    for (auto &kv : keys_vec) {
        pbf.add_string(1, kv.first->c_str());
    }
    if (dim != 2) {
        pbf.add_uint32(2, dim);
    }
    uint32_t precision = std::log10(std::min(e, maxPrecision));
    if (precision != std::log10(maxPrecision)) {
        pbf.add_uint32(3, precision);
    }

    geojson.match(
        [&](const mapbox::geojson::feature_collection &features) {
            writeFeatureCollection(features, pbf);
        },
        [&](const mapbox::geojson::feature &feature) {
            writeFeature(feature, pbf);
        },
        [&](const mapbox::geojson::geometry &geometry) {
            writeFeature(geometry, pbf);
        });
    keys.clear();
    return data;
}

void Encoder::analyze(const mapbox::geojson::geojson &geojson)
{
    auto analyze_feature = [&](const mapbox::geojson::feature &f) {
        for (auto &kv : f.properties) {
            saveKey(kv.first);
        }
        analyzeGeometry(f.geometry);
    };
    geojson.match(
        [&](const mapbox::geojson::feature &f) { analyze_feature(f); },
        [&](const mapbox::geojson::geometry &g) { analyzeGeometry(g); },
        [&](const mapbox::geojson::feature_collection &fc) {
            for (auto &f : fc) {
                analyze_feature(f);
            }
        });
}

void Encoder::analyzeGeometry(const mapbox::geojson::geometry &geometry)
{
    geometry.match(
        [&](const mapbox::geojson::point &point) { analyzePoint(point); },
        [&](const mapbox::geojson::multi_point &points) {
            analyzePoints(points);
        },
        [&](const mapbox::geojson::line_string &points) {
            analyzePoints(points);
        },
        [&](const mapbox::geojson::polygon &polygon) {
            analyzeMultiLine(
                (mapbox::geojson::multi_line_string::container_type &)polygon);
        },
        [&](const mapbox::geojson::multi_line_string &lines) {
            analyzeMultiLine(lines);
        },
        [&](const mapbox::geojson::multi_polygon &polygons) {
            for (auto &polygon : polygons) {
                analyzeMultiLine(
                    (mapbox::geojson::multi_line_string::container_type &)
                        polygon);
            }
        },
        [&](const mapbox::geojson::geometry_collection &geoms) {
            for (auto &geom : geoms) {
                analyzeGeometry(geom);
            }
        },
        [&](const mapbox::geojson::empty &null) {});
}

void Encoder::analyzeMultiLine(
    const mapbox::geojson::multi_line_string::container_type &lines)
{
    for (auto &line : lines) {
        analyzePoints(line);
    }
}
void Encoder::analyzePoints(
    const mapbox::geojson::multi_point::container_type &points)
{
    for (auto &point : points) {
        analyzePoint(point);
    }
}

void Encoder::analyzePoint(const mapbox::geojson::point &point)
{
    dim = std::max(point.z != 0 ? dimXY : dimXYZ, dim);
    const double *ptr = &point.x;
    for (int i = 0; i < dim; ++i) {
        while (std::round(ptr[i] * e) / e != ptr[i] && e < maxPrecision) {
            e *= 10;
        }
    }
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
    protozero::pbf_writer pbf_sub{pbf, 4};
    for (auto &feature : geojson) {
        writeFeature(feature, pbf_sub);
    }
}

void Encoder::writeFeature(const mapbox::geojson::feature &feature, Pbf &pbf)
{
    if (!feature.geometry.is<mapbox::geojson::empty>()) {
        protozero::pbf_writer pbf_sub{pbf, 1};
        writeGeometry(feature.geometry, pbf_sub);
    }
    if (!feature.id.is<mapbox::geojson::null_value_t>()) {
        feature.id.match([&](int64_t id) { pbf.add_int64(12, id); },
                         [&](const std::string &id) { pbf.add_string(11, id); },
                         [&](const auto &) {
                             //  dbg("unhandled id type",
                             //      mapbox::geojson::stringify(feature.id));
                         });
    }
    if (!feature.properties.empty()) {
        writeProps(feature.properties, pbf);
    }
}

void Encoder::writeGeometry(const mapbox::geojson::geometry &geometry,
                            Encoder::Pbf &pbf)
{
    geometry.match(
        [&](const mapbox::geojson::point &point) {
            pbf.add_enum(1, 0);
            writePoint(point, pbf);
        },
        [&](const mapbox::geojson::multi_point &points) {
            pbf.add_enum(1, 1);
            writeLine(points, pbf);
        },
        [&](const mapbox::geojson::line_string &lines) {
            pbf.add_enum(1, 2);
            writeLine(lines, pbf);
        },
        [&](const mapbox::geojson::multi_line_string &lines) {
            pbf.add_enum(1, 3);
            writeMultiLine((LinesType &)lines, pbf);
        },
        [&](const mapbox::geojson::polygon &polygon) {
            pbf.add_enum(1, 4);
            writeMultiLine((LinesType &)polygon, pbf);
        },
        [&](const mapbox::geojson::multi_polygon &polygons) {
            pbf.add_enum(1, 5);
            writeMultiPolygon(polygons, pbf);
        },
        [&](const mapbox::geojson::geometry_collection &geometries) {
            pbf.add_enum(1, 6);
            for (auto &geom : geometries) {
                protozero::pbf_writer pbf_sub{pbf, 4};
                writeGeometry(geom, pbf_sub);
            }
        },
        [&](const auto &) { dbg("not handled geometry type"); } //
        );
}

void Encoder::writeProps(const mapbox::feature::property_map &props,
                         Encoder::Pbf &pbf)
{
    std::vector<uint32_t> indexes;
    int valueIndex = 0;
    for (auto &pair : props) {
        protozero::pbf_writer pbf_sub{pbf, 13};
        writeValue(pair.second, pbf_sub);
        indexes.push_back(keys.at(pair.first));
        indexes.push_back(valueIndex++);
    }
    pbf.add_packed_uint32(14, indexes.begin(), indexes.end());
}

void Encoder::writeValue(const mapbox::feature::value &value, Encoder::Pbf &pbf)
{
    value.match([&](bool val) { pbf.add_bool(5, val); },
                [&](uint64_t val) { pbf.add_uint64(3, val); },
                [&](int64_t val) { pbf.add_uint64(4, -val); },
                [&](double val) { pbf.add_uint64(2, val); },
                [&](const std::string &val) { pbf.add_string(1, val); },
                [&](const auto &val) {
                    // pbf.add_string(6, mapbox::geojson::stringify(value));
                });
    //
}

void Encoder::writePoint(const mapbox::geojson::point &point, Encoder::Pbf &pbf)
{
    //
}
void Encoder::writeLine(const PointsType &line, Encoder::Pbf &pbf)
{

    //
}
void Encoder::writeMultiLine(const LinesType &lines, Encoder::Pbf &pbf)
{
    //
}
void Encoder::writeMultiPolygon(const PolygonsType &polygons, Encoder::Pbf &pbf)
{
    //
}

std::string Decoder::to_printable(const std::string &pbf_bytes)
{
    // TODO
    return ::decode(pbf_bytes.data(), pbf_bytes.size(), "");
}
}
}