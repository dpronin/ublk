configure_file(cfg.json.in cfg.json @ONLY)
set(UBLK_CFG_FILE_PATH "${UBLK_CFG_DIR}/cfg.json" PARENT_SCOPE)
install(FILES ${CMAKE_BINARY_DIR}/etc/cfg.json DESTINATION ${UBLK_CFG_DIR})
