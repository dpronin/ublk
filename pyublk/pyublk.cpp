#include <pybind11/pybind11.h>
#include <pybind11/stl/filesystem.h>

#include "bdev_map_param.hpp"
#include "bdev_unmap_param.hpp"
#include "target_create_param.hpp"
#include "target_destroy_param.hpp"

#include "master.hpp"

PYBIND11_MODULE(ublk, m) {
  namespace py = pybind11;
  using namespace py::literals;

  py::class_<ublk::bdev_map_param>(m, "bdev_map_param")
      .def(py::init<>())
      .def_readwrite("bdev_suffix", &ublk::bdev_map_param::bdev_suffix)
      .def_readwrite("target_name", &ublk::bdev_map_param::target_name)
      .def_readwrite("read_only", &ublk::bdev_map_param::read_only);

  py::class_<ublk::bdev_unmap_param>(m, "bdev_unmap_param")
      .def(py::init<>())
      .def_readwrite("bdev_suffix", &ublk::bdev_unmap_param::bdev_suffix);

  py::class_<ublk::target_create_param>(m, "target_create_param")
      .def(py::init<>())
      .def_readwrite("name", &ublk::target_create_param::name)
      .def_readwrite("capacity_sectors",
                     &ublk::target_create_param::capacity_sectors)
      .def_readwrite("path", &ublk::target_create_param::path);

  py::class_<ublk::target_destroy_param>(m, "target_destroy_param")
      .def(py::init<>())
      .def_readwrite("name", &ublk::target_destroy_param::name);

  py::class_<ublk::Master>(m, "Master")
      .def(py::init<>())
      .def("map", &ublk::Master::map, "param"_a)
      .def("unmap", &ublk::Master::unmap, "param"_a)
      .def("create", &ublk::Master::create, "param"_a)
      .def("destroy", &ublk::Master::destroy, "param"_a);
}
