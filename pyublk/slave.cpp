#include "slave.hpp"

#include <algorithm>
#include <filesystem>
#include <format>
#include <initializer_list>
#include <memory>
#include <ranges>
#include <span>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

#include <linux/ublkdrv/cmd.h>
#include <linux/ublkdrv/cmd_ack.h>
#include <linux/ublkdrv/cmdb.h>
#include <linux/ublkdrv/cmdb_ack.h>
#include <linux/ublkdrv/def.h>
#include <linux/ublkdrv/mapping.h>

#include <boost/asio/io_context.hpp>
#include <boost/lexical_cast.hpp>

#include "cmd_acknowledger.hpp"
#include "cmd_handler_factory.hpp"
#include "file.hpp"
#include "handler.hpp"
#include "mem.hpp"
#include "page.hpp"
#include "qublkcmd.hpp"

namespace {

auto maps_fetch(std::string_view bdev_name) {
  std::vector<std::filesystem::path> maps;

  auto maps_rng =
      std::views::filter(std::filesystem::directory_iterator(
                             std::format("/sys/block/{}", bdev_name)),
                         [](auto const &dir_e) {
                           return dir_e.is_directory() &&
                                  dir_e.path().filename().string().starts_with(
                                      "uio");
                         }) |
      std::views::filter([](auto const &uio) {
        auto const uio_name =
            ublk::read_from_as<std::string>(uio.path() / "name");
        static std::initializer_list<const char *> dir_list{
            UBLKDRV_UIO_KERNEL_TO_USER_DIR_SUFFIX,
            UBLKDRV_UIO_USER_TO_KERNEL_DIR_SUFFIX,
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

  for (auto &&maps_in : maps_rng)
    std::ranges::copy(maps_in, std::back_inserter(maps));

  return maps;
}

} // namespace

namespace ublk::slave {

void run(slave_param const &param) {
  struct ublk_structured_maps {
    std::unique_ptr<qublkcmd_t> p_qcmd;
    std::unique_ptr<qublkcmd_ack_t> p_qcmd_ack;
    std::shared_ptr<ublkdrv_cellc const> p_cellc;
    ublk::mem_t<std::byte> p_cells;
    size_t cells_sz;
  } structured_maps;

  auto maps = maps_fetch(UBLKDRV_PREFIX + param.bdev_suffix);

  auto cellc_map_it =
      std::ranges::find(maps, UBLKDRV_UIO_MEM_CELLC_NAME, [](auto const &map) {
        return read_from_as<std::string>(map / "name");
      });
  if (std::ranges::end(maps) == cellc_map_it)
    throw std::runtime_error("no map with cell configuration");

  std::iter_swap(cellc_map_it, maps.begin());

  evpaths_t uio_devs;

  for (auto const &map : maps) {
    auto const &sz_path = map / "size";
    auto const off_factor =
        boost::lexical_cast<unsigned>(map.filename().string().substr(3));
    auto const map_name = read_from_as<std::string>(map / "name");
    auto const uio_sys_path = map.parent_path().parent_path();
    auto const uio_dev_path = "/dev" / uio_sys_path.filename();
    auto const uio_name = read_from_as<std::string>(uio_sys_path / "name");
    if (uio_name.ends_with(UBLKDRV_UIO_KERNEL_TO_USER_DIR_SUFFIX)) {
      if (UBLKDRV_UIO_MEM_CMDB_NAME == map_name) {
        auto cmdb =
            std::shared_ptr<ublkdrv_cmdb const>{mmap_shared<ublkdrv_cmdb>(
                read_from_as<size_t>(sz_path, std::hex), PROT_READ,
                off_factor * get_page_size(), uio_dev_path)};
        auto *p_cmdb_raw = cmdb.get();

        using TH = decltype(structured_maps.p_cellc->cmdb_head);
        using TT = decltype(p_cmdb_raw->tail);

        structured_maps.p_qcmd = std::unique_ptr<qublkcmd_t>{new qublkcmd_t{
            pos_t<TH>{new TH{structured_maps.p_cellc->cmdb_head},
                      std::default_delete<TH>{}},
            pos_t<TT const>{&p_cmdb_raw->tail,
                            [cmdb]([[maybe_unused]] auto *p) {}},
            std::span{p_cmdb_raw->cmds, p_cmdb_raw->cmds_len},
        }};
      } else if (UBLKDRV_UIO_MEM_CELLC_NAME == map_name) {
        structured_maps.p_cellc = mmap_shared<ublkdrv_cellc const>(
            read_from_as<size_t>(sz_path, std::hex), PROT_READ,
            off_factor * get_page_size(), uio_dev_path);
      } else if (UBLKDRV_UIO_MEM_CELLS_NAME == map_name) {
        structured_maps.cells_sz = read_from_as<size_t>(sz_path, std::hex);
        structured_maps.p_cells = mmap_shared<std::byte>(
            structured_maps.cells_sz, PROT_READ | PROT_WRITE,
            off_factor * get_page_size(), uio_dev_path);
      }
      uio_devs[UBLKDRV_UIO_KERNEL_TO_USER_DIR_SUFFIX] = uio_dev_path;
    } else {
      if (UBLKDRV_UIO_MEM_CMDB_NAME == map_name) {
        auto cmdb =
            std::shared_ptr<ublkdrv_cmdb_ack>{mmap_shared<ublkdrv_cmdb_ack>(
                read_from_as<size_t>(sz_path, std::hex), PROT_READ | PROT_WRITE,
                off_factor * get_page_size(), uio_dev_path)};
        auto *p_cmdb_raw = cmdb.get();
        auto *p_cellc_raw = structured_maps.p_cellc.get();

        using TH = decltype(structured_maps.p_cellc->cmdb_ack_head);
        using TT = decltype(p_cmdb_raw->tail);

        structured_maps
            .p_qcmd_ack = std::unique_ptr<qublkcmd_ack_t>{new qublkcmd_ack_t{
            pos_t<TH const>{
                &p_cellc_raw->cmdb_ack_head,
                [cellc = structured_maps.p_cellc]([[maybe_unused]] auto *p) {}},
            pos_t<TT>{&p_cmdb_raw->tail, [cmdb]([[maybe_unused]] auto *p) {}},
            std::span<ublkdrv_cmd_ack>{p_cmdb_raw->cmds, p_cmdb_raw->cmds_len},
        }};
      }
      uio_devs[UBLKDRV_UIO_USER_TO_KERNEL_DIR_SUFFIX] = uio_dev_path;
    }
  }

  if (!structured_maps.p_qcmd || !structured_maps.p_cellc ||
      !structured_maps.p_cells || !structured_maps.p_qcmd_ack)
    _exit(EXIT_FAILURE);

  auto fd_notify = uptrwd<const int>{};

  if (auto it = uio_devs.find(UBLKDRV_UIO_USER_TO_KERNEL_DIR_SUFFIX);
      it != uio_devs.end()) {

    fd_notify = open(it->second, O_WRONLY);
  }

  if (!fd_notify)
    _exit(EXIT_FAILURE);

  auto handler = CmdHandlerFactory{}.create_unique(
      param.handler,
      {structured_maps.p_cellc->cellds, structured_maps.p_cellc->cellds_len},
      {structured_maps.p_cells.get(), structured_maps.cells_sz},
      std::make_unique<CmdAcknowledger>(std::move(structured_maps.p_qcmd_ack),
                                        std::move(fd_notify)));

  auto io_ctx = boost::asio::io_context{};

  async_start(uio_devs, io_ctx, std::move(structured_maps.p_qcmd),
              std::move(handler));

  io_ctx.run();

  spdlog::info("{} finished", getpid());

  _exit(0);
}

} // namespace ublk::slave
