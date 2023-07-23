if (NOT UBLK_LOG_DIR)
    set(UBLK_LOG_DIR "/var/log/ublk")
endif()

set(UBLK_CLI_LOG_DIR "${SDS_LOG_DIR}/cli")

if (NOT UBLK_CFG_DIR)
    set(UBLK_CFG_DIR "/etc/ublk")
endif()

set(UBLK_CFG_FILE_PATH_DEFAULT "${UBLK_CFG_DIR}/cfg.json")

if (NOT UBLK_LOG_LEVEL_DEFAULT)
    set(UBLK_LOG_LEVEL_DEFAULT "info")
endif()

if (NOT UBLK_LOG_LEVEL_FLUSH_DEFAULT)
    set(UBLK_LOG_LEVEL_FLUSH_DEFAULT ${UBLK_LOG_LEVEL_DEFAULT})
endif()
