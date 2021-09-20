#include "geobuf/geobuf.hpp"

#include <protozero/pbf_builder.hpp>
#include <protozero/pbf_reader.hpp>

#define DBG_MACRO_NO_WARNING
#include "dbg.h"

constexpr uint32_t maxPrecision = 1e6;
constexpr uint32_t dimXY = 2;
constexpr uint32_t dimXYZ = 2;

namespace mapbox
{
namespace geobuf
{
Encoder::Pbf Encoder::encode(const mapbox::geojson::geojson &geojson)
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

    Encoder::Pbf pbf;
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
    return pbf;
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
    for (auto &feature : geojson) {
        protozero::pbf_writer pbf_sub{pbf, 4};
        // writeFeature(feature, pbf_sub);
    }
}

void Encoder::writeFeature(const mapbox::geojson::feature &feature, Pbf &pbf)
{
    if (!feature.geometry.is<mapbox::geojson::empty>()) {
        protozero::pbf_writer pbf_sub{pbf, 1};
        writeGeometry(feature.geometry, pbf_sub);
    }
    // using identifier = mapbox::util::variant<null_value_t, uint64_t, int64_t,
    // double, std::string>;
    if (!feature.id.is<mapbox::geojson::null_value_t>()) {
        dbg("oops id");
        // feature.id.match([&](const std::string &id) { pbf.add_string(11, id);
        // },
        //                  [&](const uint64_t &id) {
        //                      // pbf.add_string(11, id);
        //                  },
        //                  [&](const int64_t &id) {
        //                      // pbf.add_string(11, id);
        //                  },
        //                  [&](const double &id) {
        //                      // pbf.add_string(11, id);
        //                  });
    }
    if (!feature.properties.empty()) {
        writeProps(feature.properties, pbf);
    }
}

void Encoder::writeGeometry(const mapbox::geojson::geometry &geojson,
                            Encoder::Pbf &pbf)
{
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
    // null_value_t, bool, uint64_t, int64_t, double, std::string,
    // mapbox::util::recursive_wrapper<std::vector<value>>,
    // mapbox::util::recursive_wrapper<std::unordered_map<std::string, value>>>;
    // value.match([&](const bool &val) { pbf.add_bool(5, val); },
    //             [&](const uint64_t &val) { pbf.add_uint64(3, val); },
    //             [&](const int64_t &val) { pbf.add_uint64(4, -val); },
    //             [&](const double &val) { pbf.add_uint64(2, val); },
    //             [&](const std::string &val) { pbf.add_string(6, val); });
    //
}
}
}