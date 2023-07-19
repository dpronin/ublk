#include "slave.hpp"

#include <algorithm>
#include <filesystem>
#include <initializer_list>
#include <memory>
#include <ranges>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

#include <linux/ublk/cmd.h>
#include <linux/ublk/cmd_ack.h>
#include <linux/ublk/cmdb.h>
#include <linux/ublk/cmdb_ack.h>
#include <linux/ublk/mapping.h>
#include <linux/ublk/ublk.h>

#include <boost/interprocess/mapped_region.hpp>
#include <boost/lexical_cast.hpp>

#include "qublkcmd.hpp"

#include "file.hpp"
#include "handler.hpp"
#include "mem.hpp"
#include "sysfs_attr.hpp"

#include "cmd_acknowledger.hpp"
#include "cmd_handler_factory.hpp"
#include "dummy_flush_handler.hpp"
#include "dummy_read_handler.hpp"
#include "dummy_write_handler.hpp"
#include "flush_handler.hpp"
#include "flush_handler_interface.hpp"
#include "read_handler.hpp"
#include "read_handler_interface.hpp"
#include "write_handler.hpp"
#include "write_handler_interface.hpp"

namespace {

using pcmdb_t = cfq::uptrwd<ublk_cmdb>;
using pcmdb_ack_t = cfq::uptrwd<ublk_cmdb_ack>;
using pcellc_t = std::shared_ptr<ublk_cellc const>;
using mem_bytes_t = cfq::mem_t<std::byte>;

} // namespace

namespace cfq {
namespace slave {

void run(bdev_create_param const &param) {
  auto const page_size = boost::interprocess::mapped_region::get_page_size();

  auto maps_rng =
      std::views::filter(
          std::filesystem::directory_iterator("/sys/block/" UBLK_PREFIX +
                                              param.bdev_suffix),
          [](auto const &dir_e) {
            return dir_e.is_directory() &&
                   dir_e.path().filename().string().starts_with("uio");
          }) |
      std::views::filter([](auto const &uio) {
        auto const uio_name = get_sysfs_attr<std::string>(uio.path() / "name");
        static std::initializer_list<const char *> dir_list{
            UBLK_UIO_KERNEL_TO_USER_DIR_SUFFIX,
            UBLK_UIO_USER_TO_KERNEL_DIR_SUFFIX,
        };
        return std::ranges::any_of(dir_list, [&uio_name](auto const *p_dir) {
          return uio_name.ends_with(p_dir);
        });
      }) |
      std::views::transform([](auto const &uio) {
        std::vector<std::filesystem::path> out;
        auto maps_dir =
            std::filesystem::directory_iterator(uio.path() / "maps");
        auto maps = maps_dir | std::views::filter([](auto const &map) {
                      return map.is_directory() &&
                             map.path().filename().string().starts_with("map");
                    });
        std::ranges::copy(maps, std::back_inserter(out));
        return out;
      });

  struct ublk_req_maps {
    std::unique_ptr<qublkcmd_t> p_qcmd;
    pcellc_t p_cellc;
    mem_bytes_t p_cells;
    size_t cells_sz;
  } req;

  struct ublk_rsp_maps {
    std::unique_ptr<qublkcmd_ack_t> p_qcmd;
  } rsp;

  std::vector<std::filesystem::path> maps;
  for (auto &&maps_in : maps_rng)
    std::ranges::copy(maps_in, std::back_inserter(maps));

  auto cellc_map_it = std::ranges::find_if(maps, [](auto const &map) {
    return UBLK_UIO_MEM_CELLC_NAME == get_sysfs_attr<std::string>(map / "name");
  });
  if (std::ranges::end(maps) == cellc_map_it)
    throw std::runtime_error("no map with cell configuration");

  std::iter_swap(cellc_map_it, maps.begin());

  evpaths_t uio_devs;

  for (auto const &map : maps) {
    auto const &sz_path = map / "size";
    auto const off_factor =
        boost::lexical_cast<unsigned>(map.filename().string().substr(3));
    auto const map_name = get_sysfs_attr<std::string>(map / "name");
    auto const uio_sys_path = map.parent_path().parent_path();
    auto const uio_dev_path = "/dev" / uio_sys_path.filename();
    auto const uio_name = get_sysfs_attr<std::string>(uio_sys_path / "name");
    if (uio_name.ends_with(UBLK_UIO_KERNEL_TO_USER_DIR_SUFFIX)) {
      if (UBLK_UIO_MEM_CMDB_NAME == map_name) {
        auto cmdb = std::shared_ptr<ublk_cmdb const>{map_shared<ublk_cmdb>(
            get_sysfs_attr<size_t>(sz_path, std::hex), PROT_READ,
            off_factor * page_size, uio_dev_path)};
        auto *p_cmdb_raw = cmdb.get();

        using TH = decltype(req.p_cellc->cmdb_head);
        using TT = decltype(p_cmdb_raw->tail);

        req.p_qcmd = std::unique_ptr<qublkcmd_t>{new qublkcmd_t{
            pos_t<TH>{new TH{req.p_cellc->cmdb_head},
                      std::default_delete<TH>{}},
            pos_t<TT const>{&p_cmdb_raw->tail, [cmdb](auto *p) {}},
            std::span{p_cmdb_raw->cmds, p_cmdb_raw->cmds_len},
        }};
      } else if (UBLK_UIO_MEM_CELLC_NAME == map_name) {
        req.p_cellc = map_shared<ublk_cellc const>(
            get_sysfs_attr<size_t>(sz_path, std::hex), PROT_READ,
            off_factor * page_size, uio_dev_path);
      } else if (UBLK_UIO_MEM_CELLS_NAME == map_name) {
        req.cells_sz = get_sysfs_attr<size_t>(sz_path, std::hex);
        req.p_cells =
            map_shared<std::byte>(req.cells_sz, PROT_READ | PROT_WRITE,
                                  off_factor * page_size, uio_dev_path);
      }
      uio_devs[UBLK_UIO_KERNEL_TO_USER_DIR_SUFFIX] = uio_dev_path;
    } else {
      if (UBLK_UIO_MEM_CMDB_NAME == map_name) {
        auto cmdb = std::shared_ptr<ublk_cmdb_ack>{map_shared<ublk_cmdb_ack>(
            get_sysfs_attr<size_t>(sz_path, std::hex), PROT_READ | PROT_WRITE,
            off_factor * page_size, uio_dev_path)};
        auto *p_cmdb_raw = cmdb.get();
        auto *p_cellc_raw = req.p_cellc.get();

        using TH = decltype(req.p_cellc->cmdb_ack_head);
        using TT = decltype(p_cmdb_raw->tail);

        rsp.p_qcmd = std::unique_ptr<qublkcmd_ack_t>{new qublkcmd_ack_t{
            pos_t<TH const>{&p_cellc_raw->cmdb_ack_head,
                            [cellc = req.p_cellc](auto *p) {}},
            pos_t<TT>{&p_cmdb_raw->tail, [cmdb](auto *p) {}},
            std::span<ublk_cmd_ack>{p_cmdb_raw->cmds, p_cmdb_raw->cmds_len},
        }};
      }
      uio_devs[UBLK_UIO_USER_TO_KERNEL_DIR_SUFFIX] = uio_dev_path;
    }
  }

  if (!req.p_qcmd || !req.p_cellc || !req.p_cells || !rsp.p_qcmd)
    _exit(EXIT_FAILURE);

  auto reader = std::shared_ptr<IReadHandler>{};
  auto writer = std::shared_ptr<IWriteHandler>{};
  auto flusher = std::shared_ptr<IFlushHandler>{};

  if (param.target != "dummy") {
    auto fd_target = std::shared_ptr{open(param.target, O_RDWR | O_CREAT)};
    if (!fd_target)
      _exit(EXIT_FAILURE);

    reader = std::make_shared<ReadHandler>(fd_target);
    writer = std::make_shared<WriteHandler>(fd_target);
    flusher = std::make_shared<FlushHandler>(fd_target);
  } else {
    reader = std::make_shared<DummyReadHandler>();
    writer = std::make_shared<DummyWriteHandler>();
    flusher = std::make_shared<DummyFlushHandler>();
  }

  auto fd_notify = uptrwd<const int>{};

  if (auto it = uio_devs.find(UBLK_UIO_USER_TO_KERNEL_DIR_SUFFIX);
      it != uio_devs.end()) {

    fd_notify = open(it->second, O_WRONLY);
    if (!fd_notify)
      _exit(EXIT_FAILURE);
  } else {
    _exit(EXIT_FAILURE);
  }

  spdlog::set_pattern("[handler %P] [%^%l%$]: %v");
  spdlog::info("started: {}", param.target.string());

  auto handler = CmdHandlerFactory{}.create_unique(
      std::move(req.p_cellc), {req.p_cells.get(), req.cells_sz},
      std::move(reader), std::move(writer), std::move(flusher),
      std::make_unique<CmdAcknowledger>(std::move(rsp.p_qcmd),
                                        std::move(fd_notify)));

  auto r = cfq::handler(*req.p_qcmd, uio_devs, *handler);

  spdlog::info("finished, err {}", r);

  _exit(r);
}

} // namespace slave
} // namespace cfq
