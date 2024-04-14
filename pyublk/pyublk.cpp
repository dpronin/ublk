#include <exception>
#include <format>
#include <iostream>
#include <memory>
#include <stdexcept>

#include <pybind11/embed.h>
#include <pybind11/eval.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/stl/filesystem.h>

#include "bdev_map_param.hpp"
#include "bdev_unmap_param.hpp"
#include "target_create_param.hpp"
#include "target_destroy_param.hpp"

#include "master.hpp"

namespace py = pybind11;

PYBIND11_EMBEDDED_MODULE(ublk, m) {
  using namespace py::literals;

  m.def("version", [] { return ublk::kVersion; });

  py::class_<std::filesystem::path>(m, "path").def(py::init<std::string>());
  py::implicitly_convertible<std::string, std::filesystem::path>();

  py::class_<ublk::cache_cfg>(m, "cache_cfg")
      .def(py::init([] -> ublk::cache_cfg {
        return {
            .len_sectors = {},
            .write_through_enable = true,
        };
      }))
      .def_readwrite("len_sectors", &ublk::cache_cfg::len_sectors)
      .def_readwrite("write_through_enable",
                     &ublk::cache_cfg::write_through_enable);

  py::class_<ublk::bdev_map_param>(m, "bdev_map_param")
      .def(py::init([] -> ublk::bdev_map_param {
        return {
            .bdev_suffix = {},
            .target_name = {},
            .read_only = false,
        };
      }))
      .def_readwrite("bdev_suffix", &ublk::bdev_map_param::bdev_suffix)
      .def_readwrite("target_name", &ublk::bdev_map_param::target_name)
      .def_readwrite("read_only", &ublk::bdev_map_param::read_only);

  py::class_<ublk::bdev_unmap_param>(m, "bdev_unmap_param")
      .def(py::init<>())
      .def_readwrite("bdev_suffix", &ublk::bdev_unmap_param::bdev_suffix);

  py::class_<ublk::target_null_cfg>(m, "target_null").def(py::init<>());

  py::class_<ublk::target_inmem_cfg>(m, "target_inmem").def(py::init<>());

  py::class_<ublk::target_default_cfg>(m, "target_default")
      .def(py::init([] -> ublk::target_default_cfg {
        return {
            .path = {},
        };
      }))
      .def_readwrite("path", &ublk::target_default_cfg::path);

  py::class_<ublk::target_raid0_cfg>(m, "target_raid0")
      .def(py::init([] -> ublk::target_raid0_cfg {
        return {
            .strip_len_sectors = 0,
            .paths = {},
        };
      }))
      .def_readwrite("strip_len_sectors",
                     &ublk::target_raid0_cfg::strip_len_sectors)
      .def_readwrite("paths", &ublk::target_raid0_cfg::paths);

  py::class_<ublk::target_raid1_cfg>(m, "target_raid1")
      .def(py::init([] -> ublk::target_raid1_cfg {
        return {
            .read_strip_len_sectors = 0,
            .paths = {},
        };
      }))
      .def_readwrite("read_len_sectors_per_path",
                     &ublk::target_raid1_cfg::read_strip_len_sectors)
      .def_readwrite("paths", &ublk::target_raid1_cfg::paths);

  py::class_<ublk::target_raid4_cfg>(m, "target_raid4")
      .def(py::init([] -> ublk::target_raid4_cfg {
        return {
            .strip_len_sectors = 0,
            .data_paths = {},
            .parity_path = {},
        };
      }))
      .def_readwrite("strip_len_sectors",
                     &ublk::target_raid4_cfg::strip_len_sectors)
      .def_readwrite("data_paths", &ublk::target_raid4_cfg::data_paths)
      .def_readwrite("parity_path", &ublk::target_raid4_cfg::parity_path);

  py::class_<ublk::target_raid5_cfg>(m, "target_raid5")
      .def(py::init([] -> ublk::target_raid5_cfg {
        return {
            .strip_len_sectors = 0,
            .paths = {},
        };
      }))
      .def_readwrite("strip_len_sectors",
                     &ublk::target_raid5_cfg::strip_len_sectors)
      .def_readwrite("paths", &ublk::target_raid5_cfg::paths);

  py::class_<ublk::target_raid10_cfg>(m, "target_raid10")
      .def(py::init([] -> ublk::target_raid10_cfg {
        return {
            .strip_len_sectors = 0,
            .raid1s = {},
        };
      }))
      .def_readwrite("strip_len_sectors",
                     &ublk::target_raid10_cfg::strip_len_sectors)
      .def_readwrite("raid1s", &ublk::target_raid10_cfg::raid1s);

  py::class_<ublk::target_raid40_cfg>(m, "target_raid40")
      .def(py::init([] -> ublk::target_raid40_cfg {
        return {
            .strip_len_sectors = 0,
            .raid4s = {},
        };
      }))
      .def_readwrite("strip_len_sectors",
                     &ublk::target_raid40_cfg::strip_len_sectors)
      .def_readwrite("raid4s", &ublk::target_raid40_cfg::raid4s);

  py::class_<ublk::target_raid50_cfg>(m, "target_raid50")
      .def(py::init([] -> ublk::target_raid50_cfg {
        return {
            .strip_len_sectors = 0,
            .raid5s = {},
        };
      }))
      .def_readwrite("strip_len_sectors",
                     &ublk::target_raid50_cfg::strip_len_sectors)
      .def_readwrite("raid5s", &ublk::target_raid50_cfg::raid5s);

  py::class_<ublk::target_create_param>(m, "target_create_param")
      .def(py::init([] -> ublk::target_create_param {
        return {
            .name = {},
            .capacity_sectors = 0,
            .cache = {},
            .target = ublk::target_null_cfg{},
        };
      }))
      .def_readwrite("name", &ublk::target_create_param::name)
      .def_readwrite("capacity_sectors",
                     &ublk::target_create_param::capacity_sectors)
      .def_readwrite("cache", &ublk::target_create_param::cache)
      .def_readwrite("target", &ublk::target_create_param::target);

  py::class_<ublk::target_destroy_param>(m, "target_destroy_param")
      .def(py::init<>())
      .def_readwrite("name", &ublk::target_destroy_param::name);

  py::class_<ublk::targets_list_param>(m, "targets_list_param")
      .def(py::init<>());

  py::class_<ublk::Master, std::unique_ptr<ublk::Master, py::nodelete>>(
      m, "Master")
      .def(py::init([] -> std::unique_ptr<ublk::Master, py::nodelete> {
        return {&ublk::Master::instance(), py::nodelete{}};
      }))
      .def("map", &ublk::Master::map, "param"_a)
      .def("unmap", &ublk::Master::unmap, "param"_a)
      .def("create", &ublk::Master::create, "param"_a)
      .def("destroy", &ublk::Master::destroy, "param"_a)
      .def("list", &ublk::Master::list, "param"_a);
}

int main(int argc [[maybe_unused]], const char *argv[]) try {
  py::scoped_interpreter guard{};
  py::object scope = py::module_::import("__main__").attr("__dict__");
  py::eval_file(argv[1], scope);
} catch (std::exception const &ex) {
  std::cerr << std::format("error occurred: {}", ex.what()) << std::endl;
} catch (...) {
  std::cerr << "unknown error occurred" << std::endl;
}
