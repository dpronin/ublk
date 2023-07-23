configure_file(cfg.json.in cfg.json @ONLY)
set(UBLK_CFG_FILE_PATH_DEFAULT "${UBLK_CFG_DIR}/cfg.json")
install(FILES ${CMAKE_BINARY_DIR}/etc/cfg.json DESTINATION ${UBLK_CFG_DIR})
