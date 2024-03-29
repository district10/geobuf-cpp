#include <mapbox/geojson.hpp>

#include <pybind11/eigen.h>
#include <pybind11/iostream.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>

#include "geobuf/geojson_helpers.hpp"
#include "geobuf/pybind11_helpers.hpp"
#include "geobuf/rapidjson_helpers.hpp"

#include <fstream>
#include <iostream>

namespace cubao
{
namespace py = pybind11;
using namespace pybind11::literals;
using rvp = py::return_value_policy;

using PropertyMap = mapbox::geojson::value::object_type;

void bind_geojson(py::module &geojson)
{
#define is_geojson_type(geojson_type)                                          \
    .def("is_" #geojson_type, [](const mapbox::geojson::geojson &self) {       \
        return self.is<mapbox::geojson::geojson_type>();                       \
    })
#define as_geojson_type(geojson_type)                                          \
    .def(                                                                      \
        "as_" #geojson_type,                                                   \
        [](mapbox::geojson::geojson &self)                                     \
            -> mapbox::geojson::geojson_type & {                               \
            return self.get<mapbox::geojson::geojson_type>();                  \
        },                                                                     \
        rvp::reference_internal)

    py::class_<mapbox::geojson::geojson>(geojson, "GeoJSON", py::module_local())
        is_geojson_type(geometry)           //
        is_geojson_type(feature)            //
        is_geojson_type(feature_collection) //
        as_geojson_type(geometry)           //
        as_geojson_type(feature)            //
        as_geojson_type(feature_collection) //
                                            //
            .def(py::self == py::self)      //
            .def(py::self != py::self)      //
        //
        ;

    py::bind_vector<mapbox::geojson::multi_point::container_type>(geojson,
                                                                  "coordinates")
        .def(
            "as_numpy",
            [](std::vector<mapbox::geojson::point> &self)
                -> Eigen::Map<RowVectors> {
                return Eigen::Map<RowVectors>(&self[0].x, self.size(), 3);
            },
            rvp::reference_internal)
        .def("to_numpy",
             [](const std::vector<mapbox::geojson::point> &self) -> RowVectors {
                 return Eigen::Map<const RowVectors>(&self[0].x, self.size(),
                                                     3);
             }) //
        .def(
            "from_numpy",
            [](std::vector<mapbox::geojson::point> &self,
               Eigen::Ref<const MatrixXdRowMajor> points)
                -> std::vector<mapbox::geojson::point> & {
                eigen2geom(points, self);
                return self;
            },
            rvp::reference_internal)
        //
        ;

#define is_geometry_type(geom_type)                                            \
    .def("is_" #geom_type, [](const mapbox::geojson::geometry &self) {         \
        return self.is<mapbox::geojson::geom_type>();                          \
    })
#define as_geometry_type(geom_type)                                            \
    .def(                                                                      \
        "as_" #geom_type,                                                      \
        [](mapbox::geojson::geometry &self) -> mapbox::geojson::geom_type & {  \
            return self.get<mapbox::geojson::geom_type>();                     \
        },                                                                     \
        rvp::reference_internal)

#define copy_deepcopy_clone(Type)                                              \
    .def("__copy__", [](const Type &self, py::dict) -> Type { return self; })  \
        .def(                                                                  \
            "__deepcopy__",                                                    \
            [](const Type &self, py::dict) -> Type { return self; }, "memo"_a) \
        .def("clone", [](const Type &self) -> Type { return self; })

    using GeometryBase = mapbox::geometry::geometry_base<double, std::vector>;
    py::class_<GeometryBase>(geojson, "GeometryBase", py::module_local());
    py::class_<mapbox::geojson::geometry, GeometryBase>(geojson, "Geometry",
                                                        py::module_local())
        .def(py::init<>())
        .def(py::init([](const mapbox::geojson::point &g) { return g; }))
        .def(py::init([](const mapbox::geojson::multi_point &g) { return g; }))
        .def(py::init([](const mapbox::geojson::line_string &g) { return g; }))
        .def(py::init(
            [](const mapbox::geojson::multi_line_string &g) { return g; }))
        .def(py::init([](const mapbox::geojson::polygon &g) { return g; }))
        .def(
            py::init([](const mapbox::geojson::multi_polygon &g) { return g; }))
        .def(py::init(
            [](const mapbox::geojson::geometry_collection &g) { return g; }))
        .def(py::init([](const mapbox::geojson::geometry &g) { return g; }))
        // check geometry type
        is_geometry_type(empty)               //
        is_geometry_type(point)               //
        is_geometry_type(line_string)         //
        is_geometry_type(polygon)             //
        is_geometry_type(multi_point)         //
        is_geometry_type(multi_line_string)   //
        is_geometry_type(multi_polygon)       //
        is_geometry_type(geometry_collection) //
        // convert geometry type
        as_geometry_type(point)               //
        as_geometry_type(line_string)         //
        as_geometry_type(polygon)             //
        as_geometry_type(multi_point)         //
        as_geometry_type(multi_line_string)   //
        as_geometry_type(multi_polygon)       //
        as_geometry_type(geometry_collection) //
        .def(
            "__getitem__",
            [](mapbox::geojson::geometry &self,
               const std::string &key) -> mapbox::geojson::value & {
                return self.custom_properties.at(key);
            },
            "key"_a, rvp::reference_internal) //
        .def(
            "get",
            [](mapbox::geojson::geometry &self,
               const std::string &key) -> mapbox::geojson::value * {
                auto &props = self.custom_properties;
                auto itr = props.find(key);
                if (itr == props.end()) {
                    return nullptr;
                }
                return &itr->second;
            },
            "key"_a, rvp::reference_internal)
        .def("__setitem__",
             [](mapbox::geojson::geometry &self, const std::string &key,
                const py::object &value) {
                 self.custom_properties[key] = to_geojson_value(value);
                 return value;
             })
        .def(
            "push_back",
            [](mapbox::geojson::geometry &self,
               const mapbox::geojson::point &point)
                -> mapbox::geojson::geometry & {
                geometry_push_back(self, point);
                return self;
            },
            rvp::reference_internal)
        .def(
            "push_back",
            [](mapbox::geojson::geometry &self,
               const Eigen::VectorXd &point) -> mapbox::geojson::geometry & {
                geometry_push_back(self, point);
                return self;
            },
            rvp::reference_internal)
        .def(
            "push_back",
            [](mapbox::geojson::geometry &self,
               const mapbox::geojson::geometry &geom)
                -> mapbox::geojson::geometry & {
                geometry_push_back(self, geom);
                return self;
            },
            rvp::reference_internal)
        .def(
            "pop_back",
            [](mapbox::geojson::geometry &self,
               const mapbox::geojson::point &point)
                -> mapbox::geojson::geometry & {
                geometry_pop_back(self);
                return self;
            },
            rvp::reference_internal)
        .def(
            "clear",
            [](mapbox::geojson::geometry &self) -> mapbox::geojson::geometry & {
                geometry_clear(self);
                return self;
            },
            rvp::reference_internal)
        .def("type",
             [](const mapbox::geojson::geometry &self) {
                 return geometry_type(self);
             })
        //
        .def(
            "as_numpy",
            [](mapbox::geojson::geometry &self) -> Eigen::Map<RowVectors> {
                return as_row_vectors(self);
            },
            rvp::reference_internal)
        .def("to_numpy",
             [](const mapbox::geojson::geometry &self) -> RowVectors {
                 return as_row_vectors(self);
             }) //
        .def(
            "from_numpy",
            [](mapbox::geojson::geometry &self,
               Eigen::Ref<const MatrixXdRowMajor> points)
                -> mapbox::geojson::geometry & {
                eigen2geom(points, self);
                return self;
            },
            rvp::reference_internal) //
        // dumps, loads, from/to rapidjson
        copy_deepcopy_clone(mapbox::geojson::geometry)
        .def(py::pickle(
            [](const mapbox::geojson::geometry &self) {
                return to_python(self);
            },
            [](py::object o) -> mapbox::geojson::geometry {
                auto json = to_rapidjson(o);
                return mapbox::geojson::convert<mapbox::geojson::geometry>(
                    json);
            }))
        .def_property_readonly(
            "__geo_interface__",
            [](const mapbox::geojson::geometry &self) -> py::object {
                return to_python(self);
            })
        .def("from_rapidjson",
             [](mapbox::geojson::geometry &self,
                const RapidjsonValue &json) -> mapbox::geojson::geometry & {
                 self =
                     mapbox::geojson::convert<mapbox::geojson::geometry>(json);
                 return self;
             })
        .def("to_rapidjson",
             [](const mapbox::geojson::geometry &self) {
                 RapidjsonAllocator allocator;
                 auto json = mapbox::geojson::convert(self, allocator);
                 return json;
             })
        .def(py::self == py::self) //
        .def(py::self != py::self) //
        .def("__call__",
             [](const mapbox::geojson::geometry &self) {
                 return to_python(self);
             })
        //
        ;

    py::class_<mapbox::geojson::point>(geojson, "Point", py::module_local())
        .def(py::init<>())
        .def(py::init<double, double, double>(), "x"_a, "y"_a, "z"_a = 0.0)
        .def(py::init([](const Eigen::VectorXd &p) {
            return mapbox::geojson::point(p[0], p[1],
                                          p.size() > 2 ? p[2] : 0.0);
        }))
        .def(
            "as_numpy",
            [](mapbox::geojson::point &self) -> Eigen::Map<Eigen::Vector3d> {
                return Eigen::Vector3d::Map(&self.x);
            },
            rvp::reference_internal)
        .def("to_numpy",
             [](const mapbox::geojson::point &self) -> Eigen::Vector3d {
                 return Eigen::Vector3d::Map(&self.x);
             })
        .def(
            "from_numpy",
            [](mapbox::geojson::point &self,
               const Eigen::Vector3d &p) -> mapbox::geojson::point & {
                Eigen::Vector3d::Map(&self.x) = p;
                return self;
            },
            rvp::reference_internal)
        .def_property(
            "x", [](const mapbox::geojson::point &self) { return self.x; },
            [](mapbox::geojson::point &self, double value) { self.x = value; })
        .def_property(
            "y", [](const mapbox::geojson::point &self) { return self.y; },
            [](mapbox::geojson::point &self, double value) { self.y = value; })
        .def_property(
            "z", [](const mapbox::geojson::point &self) { return self.z; },
            [](mapbox::geojson::point &self, double value) { self.z = value; })
        .def(
            "__getitem__",
            [](mapbox::geojson::point &self, int index) -> double {
                return *(&self.x + (index >= 0 ? index : index + 3));
            },
            "index"_a)
        .def(
            "__setitem__",
            [](mapbox::geojson::point &self, int index, double v) {
                *(&self.x + (index >= 0 ? index : index + 3)) = v;
                return v;
            },
            "index"_a, "value"_a) copy_deepcopy_clone(mapbox::geojson::point)
        .def(py::pickle(
            [](const mapbox::geojson::point &self) {
                return to_python(mapbox::geojson::geometry(self));
            },
            [](py::object o) -> mapbox::geojson::point {
                auto json = to_rapidjson(o);
                return mapbox::geojson::convert<mapbox::geojson::geometry>(json)
                    .get<mapbox::geojson::point>();
            }))
        .def_property_readonly(
            "__geo_interface__",
            [](const mapbox::geojson::point &self) -> py::object {
                return to_python(self);
            })
        .def("from_rapidjson",
             [](mapbox::geojson::point &self,
                const RapidjsonValue &json) -> mapbox::geojson::point & {
                 self =
                     mapbox::geojson::convert<mapbox::geojson::geometry>(json)
                         .get<mapbox::geojson::point>();
                 return self;
             })
        .def("to_rapidjson",
             [](const mapbox::geojson::point &self) {
                 RapidjsonAllocator allocator;
                 auto json = mapbox::geojson::convert(
                     mapbox::geojson::geometry{self}, allocator);
                 return json;
             })
        .def(py::self == py::self) //
        .def(py::self != py::self) //
        .def("__call__",
             [](const mapbox::geojson::point &self) { return to_python(self); })
        //
        ;

#define BIND_FOR_VECTOR_POINT_TYPE(geom_type)                                  \
    .def(py::init([](Eigen::Ref<const MatrixXdRowMajor> points) {              \
        mapbox::geojson::geom_type self;                                       \
        eigen2geom(points, self);                                              \
        return self;                                                           \
    }))                                                                        \
        .def("__call__",                                                       \
             [](const mapbox::geojson::geom_type &self) {                      \
                 return to_python(self);                                       \
             })                                                                \
        .def(                                                                  \
            "__getitem__",                                                     \
            [](mapbox::geojson::geom_type &self,                               \
               int index) -> mapbox::geojson::point & {                        \
                return self[index >= 0 ? index : index + (int)self.size()];    \
            },                                                                 \
            rvp::reference_internal)                                           \
        .def("__setitem__",                                                    \
             [](mapbox::geojson::geom_type &self, int index,                   \
                const mapbox::geojson::point &p) {                             \
                 self[index >= 0 ? index : index + (int)self.size()] = p;      \
                 return p;                                                     \
             })                                                                \
        .def("__setitem__",                                                    \
             [](mapbox::geojson::geom_type &self, int index,                   \
                const Eigen::VectorXd &p) {                                    \
                 index = index >= 0 ? index : index + (int)self.size();        \
                 self[index].x = p[0];                                         \
                 self[index].y = p[1];                                         \
                 self[index].z = p.size() > 2 ? p[2] : 0.0;                    \
                 return p;                                                     \
             })                                                                \
        .def("__len__",                                                        \
             [](const mapbox::geojson::geom_type &self) -> int {               \
                 return self.size();                                           \
             })                                                                \
        .def(                                                                  \
            "__iter__",                                                        \
            [](mapbox::geojson::geom_type &self) {                             \
                return py::make_iterator(self.begin(), self.end());            \
            },                                                                 \
            py::keep_alive<0, 1>())                                            \
        .def("clear", &mapbox::geojson::geom_type::clear)                      \
        .def("pop_back", &mapbox::geojson::geom_type::pop_back)                \
        .def(                                                                  \
            "push_back",                                                       \
            [](mapbox::geojson::geom_type &self,                               \
               const mapbox::geojson::point &point)                            \
                -> mapbox::geojson::geom_type & {                              \
                self.push_back(point);                                         \
                return self;                                                   \
            },                                                                 \
            rvp::reference_internal)                                           \
        .def(                                                                  \
            "push_back",                                                       \
            [](mapbox::geojson::geom_type &self,                               \
               const std::array<double, 3> &xyz)                               \
                -> mapbox::geojson::geom_type & {                              \
                self.emplace_back(xyz[0], xyz[1], xyz[2]);                     \
                return self;                                                   \
            },                                                                 \
            rvp::reference_internal)                                           \
        .def(                                                                  \
            "as_numpy",                                                        \
            [](mapbox::geojson::geom_type &self) -> Eigen::Map<RowVectors> {   \
                return Eigen::Map<RowVectors>(&self[0].x, self.size(), 3);     \
            },                                                                 \
            rvp::reference_internal)                                           \
        .def("to_numpy",                                                       \
             [](const mapbox::geojson::geom_type &self) -> RowVectors {        \
                 return Eigen::Map<const RowVectors>(&self[0].x, self.size(),  \
                                                     3);                       \
             })                                                                \
        .def(                                                                  \
            "from_numpy",                                                      \
            [](mapbox::geojson::geom_type &self,                               \
               Eigen::Ref<const MatrixXdRowMajor> points)                      \
                -> mapbox::geojson::geom_type & {                              \
                eigen2geom(points, self);                                      \
                return self;                                                   \
            },                                                                 \
            rvp::reference_internal)                                           \
            copy_deepcopy_clone(mapbox::geojson::geom_type)                    \
        .def(py::pickle(                                                       \
            [](const mapbox::geojson::geom_type &self) {                       \
                return to_python(mapbox::geojson::geometry{self});             \
            },                                                                 \
            [](py::object o) -> mapbox::geojson::geom_type {                   \
                auto json = to_rapidjson(o);                                   \
                return mapbox::geojson::convert<mapbox::geojson::geometry>(    \
                           json)                                               \
                    .get<mapbox::geojson::geom_type>();                        \
            }))                                                                \
        .def_property_readonly(                                                \
            "__geo_interface__",                                               \
            [](const mapbox::geojson::geom_type &self) -> py::object {         \
                return to_python(mapbox::geojson::geometry(self));             \
            })                                                                 \
        .def("from_rapidjson",                                                 \
             [](mapbox::geojson::geom_type &self,                              \
                const RapidjsonValue &json) -> mapbox::geojson::geom_type & {  \
                 self =                                                        \
                     mapbox::geojson::convert<mapbox::geojson::geometry>(json) \
                         .get<mapbox::geojson::geom_type>();                   \
                 return self;                                                  \
             })                                                                \
        .def("to_rapidjson", [](const mapbox::geojson::geom_type &self) {      \
            RapidjsonAllocator allocator;                                      \
            auto json = mapbox::geojson::convert(                              \
                mapbox::geojson::geometry{self}, allocator);                   \
            return json;                                                       \
        })

    py::class_<mapbox::geojson::multi_point,
               std::vector<mapbox::geojson::point>>(geojson, "MultiPoint",
                                                    py::module_local())
        .def(py::init<>())                      //
        BIND_FOR_VECTOR_POINT_TYPE(multi_point) //
        .def(py::self == py::self)              //
        .def(py::self != py::self)              //
        //
        ;
    py::class_<mapbox::geojson::line_string,
               std::vector<mapbox::geojson::point>>(geojson, "LineString",
                                                    py::module_local())
        .def(py::init<>())                      //
        BIND_FOR_VECTOR_POINT_TYPE(line_string) //
        .def(py::self == py::self)              //
        .def(py::self != py::self)              //
        //
        ;

#define BIND_FOR_VECTOR_LINEAR_RING_TYPE(geom_type)                            \
    .def(py::init([](Eigen::Ref<const MatrixXdRowMajor> points) {              \
        mapbox::geojson::geom_type self;                                       \
        eigen2geom(points, self);                                              \
        return self;                                                           \
    }))                                                                        \
        .def("__call__",                                                       \
             [](const mapbox::geojson::geom_type &self) {                      \
                 return to_python(self);                                       \
             })                                                                \
        .def("__len__",                                                        \
             [](const mapbox::geojson::geom_type &self) -> int {               \
                 return self.size();                                           \
             })                                                                \
        .def(                                                                  \
            "__iter__",                                                        \
            [](mapbox::geojson::geom_type &self) {                             \
                return py::make_iterator(self.begin(), self.end());            \
            },                                                                 \
            py::keep_alive<0, 1>())                                            \
        .def("clear", &mapbox::geojson::geom_type::clear)                      \
        .def("pop_back", &mapbox::geojson::geom_type::pop_back)                \
        .def(                                                                  \
            "as_numpy",                                                        \
            [](mapbox::geojson::geom_type &self) -> Eigen::Map<RowVectors> {   \
                return as_row_vectors(self);                                   \
            },                                                                 \
            rvp::reference_internal)                                           \
        .def("to_numpy",                                                       \
             [](const mapbox::geojson::geom_type &self) -> RowVectors {        \
                 return as_row_vectors(self);                                  \
             })                                                                \
        .def(                                                                  \
            "from_numpy",                                                      \
            [](mapbox::geojson::geom_type &self,                               \
               Eigen::Ref<const MatrixXdRowMajor> points)                      \
                -> mapbox::geojson::geom_type & {                              \
                eigen2geom(points, self);                                      \
                return self;                                                   \
            },                                                                 \
            rvp::reference_internal)                                           \
            copy_deepcopy_clone(mapbox::geojson::geom_type)                    \
        .def(py::pickle(                                                       \
            [](const mapbox::geojson::geom_type &self) {                       \
                return to_python(mapbox::geojson::geometry{self});             \
            },                                                                 \
            [](py::object o) -> mapbox::geojson::geom_type {                   \
                auto json = to_rapidjson(o);                                   \
                return mapbox::geojson::convert<mapbox::geojson::geometry>(    \
                           json)                                               \
                    .get<mapbox::geojson::geom_type>();                        \
            }))                                                                \
        .def_property_readonly(                                                \
            "__geo_interface__",                                               \
            [](const mapbox::geojson::geom_type &self) -> py::object {         \
                return to_python(mapbox::geojson::geometry(self));             \
            })                                                                 \
        .def("from_rapidjson",                                                 \
             [](mapbox::geojson::geom_type &self,                              \
                const RapidjsonValue &json) -> mapbox::geojson::geom_type & {  \
                 self =                                                        \
                     mapbox::geojson::convert<mapbox::geojson::geometry>(json) \
                         .get<mapbox::geojson::geom_type>();                   \
                 return self;                                                  \
             })                                                                \
        .def("to_rapidjson", [](const mapbox::geojson::geom_type &self) {      \
            RapidjsonAllocator allocator;                                      \
            auto json = mapbox::geojson::convert(                              \
                mapbox::geojson::geometry{self}, allocator);                   \
            return json;                                                       \
        })

    py::class_<mapbox::geojson::linear_ring,
               mapbox::geojson::linear_ring::container_type>(
        geojson, "LinearRing", py::module_local()) //
        .def(py::init<>())                         //
        .def(py::self == py::self)                 //
        .def(py::self != py::self)                 //
        //
        ;

    py::bind_vector<mapbox::geojson::multi_line_string::container_type>(
        geojson, "LineStringList", py::module_local());

    py::class_<mapbox::geojson::multi_line_string,
               mapbox::geojson::multi_line_string::container_type>(
        geojson, "MultiLineString", py::module_local()) //
        .def(py::init<>())
        .def(py::init<mapbox::geojson::multi_line_string::container_type>())
        .def(py::init([](std::vector<mapbox::geojson::point> line_string) {
            return mapbox::geojson::multi_line_string({std::move(line_string)});
        })) //
        //
        BIND_FOR_VECTOR_LINEAR_RING_TYPE(multi_line_string) //
        .def(py::self == py::self)                          //
        .def(py::self != py::self)                          //
        //
        ;

    py::bind_vector<mapbox::geojson::polygon::container_type>(
        geojson, "LinearRingList", py::module_local());
    py::class_<mapbox::geojson::polygon,
               mapbox::geojson::polygon::container_type>(geojson, "Polygon",
                                                         py::module_local()) //
        .def(py::init<>())
        .def(py::init<mapbox::geojson::polygon::container_type>())
        .def(py::init([](std::vector<mapbox::geojson::point> shell) {
            return mapbox::geojson::polygon({std::move(shell)});
        })) //
        //
        BIND_FOR_VECTOR_LINEAR_RING_TYPE(polygon) //
        .def(py::self == py::self)                //
        .def(py::self != py::self)                //
        //
        ;

    py::bind_vector<mapbox::geojson::multi_polygon::container_type>(
        geojson, "PolygonList");
    py::class_<mapbox::geojson::multi_polygon,
               mapbox::geojson::multi_polygon::container_type>(
        geojson, "MultiPolygon", py::module_local()) //
        .def("__call__",
             [](const mapbox::geojson::multi_polygon &self) {
                 return to_python(self);
             })
        .def(py::init<>())
        .def(py::init<mapbox::geojson::multi_polygon>())
        .def(py::init<mapbox::geojson::multi_polygon::container_type>())
        .def(
            "as_numpy",
            [](mapbox::geojson::multi_polygon &self) -> Eigen::Map<RowVectors> {
                return as_row_vectors(self);
            },
            rvp::reference_internal)
        .def("to_numpy",
             [](const mapbox::geojson::polygon &self) -> RowVectors {
                 return as_row_vectors(self);
             }) //
        .def(
            "from_numpy",
            [](mapbox::geojson::multi_polygon &self,
               Eigen::Ref<const MatrixXdRowMajor> points)
                -> mapbox::geojson::multi_polygon & {
                eigen2geom(points, self);
                return self;
            },
            rvp::reference_internal) //
                                     //
        .def(py::init([](Eigen::Ref<const MatrixXdRowMajor> points) {
            mapbox::geojson::multi_polygon self;
            eigen2geom(points, self);
            return self;
        }))
        .def("__call__",
             [](const mapbox::geojson::multi_polygon &self) {
                 return to_python(self);
             })
        .def("__len__",
             [](const mapbox::geojson::multi_polygon &self) -> int {
                 return self.size();
             })
        .def(
            "__iter__",
            [](mapbox::geojson::multi_polygon &self) {
                return py::make_iterator(self.begin(), self.end());
            },
            py::keep_alive<0, 1>())
        .def("clear", &mapbox::geojson::multi_polygon::clear)
        .def("pop_back", &mapbox::geojson::multi_polygon::pop_back)
            copy_deepcopy_clone(mapbox::geojson::multi_polygon)
        .def(py::pickle(
            [](const mapbox::geojson::multi_polygon &self) {
                return to_python(mapbox::geojson::geometry{self});
            },
            [](py::object o) -> mapbox::geojson::multi_polygon {
                auto json = to_rapidjson(o);
                return mapbox::geojson::convert<mapbox::geojson::geometry>(json)
                    .get<mapbox::geojson::multi_polygon>();
            }))
        .def_property_readonly(
            "__geo_interface__",
            [](const mapbox::geojson::multi_polygon &self) -> py::object {
                return to_python(mapbox::geojson::geometry(self));
            })
        .def(
            "from_rapidjson",
            [](mapbox::geojson::multi_polygon &self,
               const RapidjsonValue &json) -> mapbox::geojson::multi_polygon & {
                self = mapbox::geojson::convert<mapbox::geojson::geometry>(json)
                           .get<mapbox::geojson::multi_polygon>();
                return self;
            })
        .def("to_rapidjson",
             [](const mapbox::geojson::multi_polygon &self) {
                 RapidjsonAllocator allocator;
                 auto json = mapbox::geojson::convert(
                     mapbox::geojson::geometry{self}, allocator);
                 return json;
             })
        .def(py::self == py::self) //
        .def(py::self != py::self) //
        //
        ;

    py::bind_vector<mapbox::geojson::geometry_collection::container_type>(
        geojson, "GeometryList", py::module_local());
    py::class_<mapbox::geojson::geometry_collection,
               mapbox::geojson::geometry_collection::container_type>(
        geojson, "GeometryCollection", py::module_local()) //
        .def(py::init<>());

    auto geojson_value =
        py::class_<mapbox::geojson::value>(geojson, "value", py::module_local())
            .def(py::init<>())
            .def(
                py::init([](const py::object &obj) { return to_geojson_value(obj); }))
            .def("set",
                 [](mapbox::geojson::value &self,
                    const py::object &obj) -> mapbox::geojson::value & {
                     self = to_geojson_value(obj);
                     return self;
                 },
                 rvp::reference_internal)
            .def("GetType",
                 [](const mapbox::geojson::value &self) {
                    return get_type(self);
                 })
            .def("__call__",
                 [](const mapbox::geojson::value &self) {
                     return to_python(self);
                 })
            .def("Get",
                 [](const mapbox::geojson::value &self) {
                     return to_python(self);
                 })
            .def("GetBool",
                 [](mapbox::geojson::value &self) { return self.get<bool>(); })
            .def("GetUint64",
                 [](mapbox::geojson::value &self) {
                     return self.get<uint64_t>();
                 })
            .def("GetInt64",
                 [](mapbox::geojson::value &self) {
                     return self.get<int64_t>();
                 })
            .def("GetDouble",
                 [](mapbox::geojson::value &self) -> double & {
                     return self.get<double>();
                 })
            .def("GetString",
                 [](mapbox::geojson::value &self) -> std::string & {
                     return self.get<std::string>();
                 })
            // casters
            .def("is_object",
                 [](const mapbox::geojson::value &self) {
                     return self.is<mapbox::geojson::value::object_type>();
                 })
            .def("as_object",
                 [](mapbox::geojson::value &self)
                     -> mapbox::geojson::value::object_type & {
                         return self.get<mapbox::geojson::value::object_type>();
                     },
                 rvp::reference_internal)
            .def("is_array",
                 [](const mapbox::geojson::value &self) {
                     return self.is<mapbox::geojson::value::array_type>();
                 })
            .def("as_array",
                 [](mapbox::geojson::value &self)
                     -> mapbox::geojson::value::array_type & {
                         return self.get<mapbox::geojson::value::array_type>();
                     },
                 rvp::reference_internal)
            .def("__getitem__",
                 [](mapbox::geojson::value &self,
                    int index) -> mapbox::geojson::value & {
                     auto &arr = self.get<mapbox::geojson::value::array_type>();
                     return arr[index >= 0 ? index : index + (int)arr.size()];
                 },
                 rvp::reference_internal)
            .def("__getitem__",
                 [](mapbox::geojson::value &self,
                    const std::string &key) -> mapbox::geojson::value & {
                     auto &obj =
                         self.get<mapbox::geojson::value::object_type>();
                     return obj.at(key);
                 },
                 rvp::reference_internal)                         //
            .def(
                "get",
                [](mapbox::geojson::value &self,
                    const std::string &key) -> mapbox::geojson::value * {
                     auto &obj =
                         self.get<mapbox::geojson::value::object_type>();
                    auto itr = obj.find(key);
                    if (itr == obj.end()) {
                        return nullptr;
                    }
                    return &itr->second;
                },
                "key"_a, rvp::reference_internal)
            .def("__setitem__",
                 [](mapbox::geojson::value &self, const std::string &key,
                    const py::object &value) {
                     auto &obj =
                         self.get<mapbox::geojson::value::object_type>();
                     obj[key] = to_geojson_value(value);
                     return value;
                 })
            .def("__setitem__",
                 [](mapbox::geojson::value &self, int index,
                    const py::object &value) {
                     auto &arr = self.get<mapbox::geojson::value::array_type>();
                     arr[index >= 0 ? index : index + (int)arr.size()] =
                         to_geojson_value(value);
                     return value;
                 })
            .def("keys",
                 [](mapbox::geojson::value &self) {
                     std::vector<std::string> keys;
                     auto &obj =
                         self.get<mapbox::geojson::value::object_type>();
                     return py::make_key_iterator(obj);
                 }, py::keep_alive<0, 1>())
            .def("items",
                 [](mapbox::geojson::value &self) {
                     auto &obj =
                         self.get<mapbox::geojson::value::object_type>();
                     return py::make_iterator(obj.begin(), obj.end());
                 },
                 py::keep_alive<0, 1>())

            .def("__delitem__",
                 [](mapbox::geojson::value &self, const std::string &key) {
                     auto &obj =
                         self.get<mapbox::geojson::value::object_type>();
                     return obj.erase(key);
                 })
            .def("__delitem__",
                 [](mapbox::geojson::value &self, int index) {
                     auto &arr = self.get<mapbox::geojson::value::array_type>();
                     arr.erase(arr.begin() +
                               (index >= 0 ? index : index + (int)arr.size()));
                 })
            .def("clear",
                 [](mapbox::geojson::value &self) -> mapbox::geojson::value & {
                     clear_geojson_value(self);
                     return self;
                 },
                 rvp::reference_internal)
            .def("push_back",
                 [](mapbox::geojson::value &self,
                    const py::object &value) -> mapbox::geojson::value & {
                     auto &arr = self.get<mapbox::geojson::value::array_type>();
                     arr.push_back(to_geojson_value(value));
                     return self;
                 },
                 rvp::reference_internal)
            .def("pop_back",
                 [](mapbox::geojson::value &self) -> mapbox::geojson::value & {
                     auto &arr = self.get<mapbox::geojson::value::array_type>();
                     arr.pop_back();
                     return self;
                 },
                 rvp::reference_internal)
            .def("__len__",
                 [](const mapbox::geojson::value &self) -> int {
                     return self.match(
                         [](const mapbox::geojson::value::array_type &arr) {
                             return arr.size();
                         },
                         [](const mapbox::geojson::value::object_type &obj) {
                             return obj.size();
                         },
                         [](auto &) -> int {
                            return 0;
                         });
                 })
            .def("__bool__",
                 [](const mapbox::geojson::value &self) -> bool {
                     return self.match(
                         [](const mapbox::geojson::value::object_type &obj) {
                             return !obj.empty();
                         },
                         [](const mapbox::geojson::value::array_type &arr) {
                             return !arr.empty();
                         },
                         [](const bool &b) { return b; },
                         [](const uint64_t &i) { return i != 0; },
                         [](const int64_t &i) { return i != 0; },
                         [](const double &d) { return d != 0; },
                         [](const std::string &s) { return !s.empty(); },
                         [](const mapbox::geojson::null_value_t &) {
                             return false;
                         },
                         [](auto &v) -> bool {
                            return false;
                         });
                 })
        //
        //
        ;

    // py::class_<mapbox::geojson::value::array_type>(geojson_value,
    // "array_type")
    py::bind_vector<mapbox::geojson::value::array_type>(
        geojson_value, "array_type", py::module_local())
        .def(py::init<>())
        .def(py::init([](const py::handle &arr) {
            return to_geojson_value(arr)
                .get<mapbox::geojson::value::array_type>();
        }))
        .def("clear",
             [](mapbox::geojson::value::array_type &self) { self.clear(); })
        .def(
            "__getitem__",
            [](mapbox::geojson::value::array_type &self,
               int index) -> mapbox::geojson::value & {
                return self[index >= 0 ? index : index + (int)self.size()];
            },
            rvp::reference_internal)
        .def(
            "__setitem__",
            [](mapbox::geojson::value::array_type &self, int index,
               const py::object &obj) {
                index = index < 0 ? index + (int)self.size() : index;
                self[index] = to_geojson_value(obj);
                return self[index]; // why not return obj?
            },
            rvp::reference_internal)
        //
        ;

    py::bind_map<mapbox::geojson::value::object_type>(
        geojson_value, "object_type", py::module_local())
        .def(py::init<>())
        .def(py::init([](const py::object &obj) {
            return to_geojson_value(obj)
                .get<mapbox::geojson::value::object_type>();
        }))
        .def("clear",
             [](mapbox::geojson::value::object_type &self) { self.clear(); })
        .def(
            "__setitem__",
            [](mapbox::geojson::value::object_type &self,
               const std::string &key, const py::object &obj) {
                self[key] = to_geojson_value(obj);
                return self[key];
            },
            rvp::reference_internal)
        .def(
            "keys",
            [](const mapbox::geojson::value::object_type &self) {
                return py::make_key_iterator(self.begin(), self.end());
            },
            py::keep_alive<0, 1>())
        .def(
            "items",
            [](mapbox::geojson::value::object_type &self) {
                return py::make_iterator(self.begin(), self.end());
            },
            py::keep_alive<0, 1>())
        //
        ;

    py::class_<mapbox::geojson::feature>(geojson, "Feature", py::module_local())
        .def(py::init<>())
            BIND_PY_FLUENT_ATTRIBUTE(mapbox::geojson::feature,  //
                                     mapbox::geojson::geometry, //
                                     geometry)                  //
        BIND_PY_FLUENT_ATTRIBUTE(mapbox::geojson::feature,      //
                                 PropertyMap,                   //
                                 properties)                    //
        BIND_PY_FLUENT_ATTRIBUTE(mapbox::geojson::feature,      //
                                 PropertyMap,                   //
                                 custom_properties)             //

// geometry from point, mulipoint, etc
#define GeometryFromType(geom_type)                                            \
    .def(                                                                      \
        "geometry",                                                            \
        [](mapbox::geojson::feature &self,                                     \
           const mapbox::geojson::geom_type &geometry)                         \
            -> mapbox::geojson::feature & {                                    \
            self.geometry = geometry;                                          \
            return self;                                                       \
        },                                                                     \
        #geom_type##_a, rvp::reference_internal) //
                                                 //
        GeometryFromType(point)                  //
        GeometryFromType(multi_point)            //
        GeometryFromType(line_string)            //
        GeometryFromType(multi_line_string)      //
        GeometryFromType(polygon)                //
        GeometryFromType(multi_polygon)          //
#undef GeometryFromType

        .def(
            "geometry",
            [](mapbox::geojson::feature &self,
               const py::object &obj) -> mapbox::geojson::feature & {
                auto json = to_rapidjson(obj);
                self.geometry =
                    mapbox::geojson::convert<mapbox::geojson::geometry>(json);
                return self;
            },
            rvp::reference_internal)
        .def(
            "properties",
            [](mapbox::geojson::feature &self,
               const py::object &obj) -> mapbox::geojson::feature & {
                auto json = to_rapidjson(obj);
                self.properties =
                    mapbox::geojson::convert<mapbox::feature::property_map>(
                        json);
                return self;
            },
            rvp::reference_internal)
        .def(
            "properties",
            [](mapbox::geojson::feature &self,
               const std::string &key) -> mapbox::geojson::value * {
                auto &props = self.properties;
                auto itr = props.find(key);
                if (itr == props.end()) {
                    return nullptr;
                }
                return &itr->second;
            },
            rvp::reference_internal)
        .def(
            "properties",
            [](mapbox::geojson::feature &self, const std::string &key,
               const py::object &value) -> mapbox::geojson::feature & {
                if (value.ptr() == nullptr || value.is_none()) {
                    self.properties.erase(key);
                } else {
                    self.properties[key] = to_geojson_value(value);
                }
                return self;
            },
            rvp::reference_internal)
        //
        .def(
            "__getitem__",
            [](mapbox::geojson::feature &self,
               const std::string &key) -> mapbox::geojson::value * {
                // don't try "type", "geometry", "properties"
                auto &props = self.custom_properties;
                auto itr = props.find(key);
                if (itr == props.end()) {
                    return nullptr;
                }
                return &itr->second;
            },
            rvp::reference_internal)
        .def(
            "__setitem__",
            [](mapbox::geojson::feature &self, const std::string &key,
               const py::object &value) {
                self.custom_properties[key] = to_geojson_value(value);
                return value;
            },
            rvp::reference_internal) //
        .def(
            "keys",
            [](mapbox::geojson::feature &self) {
                return py::make_key_iterator(self.custom_properties);
            },
            py::keep_alive<0, 1>())
        .def(
            "items",
            [](mapbox::geojson::feature &self) {
                return py::make_iterator(self.custom_properties.begin(),
                                         self.custom_properties.end());
            },
            py::keep_alive<0, 1>())
        .def(
            "clear",
            [](mapbox::geojson::feature &self) -> mapbox::geojson::feature & {
                self.custom_properties.clear();
                return self;
            },
            rvp::reference_internal)
        .def(py::self == py::self) //
        .def(py::self != py::self) //
        //
        .def(
            "as_numpy",
            [](mapbox::geojson::feature &self) -> Eigen::Map<RowVectors> {
                return as_row_vectors(self.geometry);
            },
            rvp::reference_internal)
        .def("to_numpy",
             [](const mapbox::geojson::feature &self) -> RowVectors {
                 return as_row_vectors(self.geometry);
             }) //
        .def("__call__",
             [](const mapbox::geojson::feature &self) {
                 return to_python(self);
             })
        //
        ;

    py::bind_vector<std::vector<mapbox::geojson::feature>>(
        geojson, "FeatureList", py::module_local())
        //
        .def("__call__", [](const std::vector<mapbox::geojson::feature> &self) {
            return to_python(self);
        });
    py::class_<mapbox::geojson::feature_collection,
               std::vector<mapbox::geojson::feature>>(
        geojson, "FeatureCollection", py::module_local())
        //
        .def("__call__", [](const mapbox::geojson::feature_collection &self) {
            return to_python(self);
        });
}
} // namespace cubao
