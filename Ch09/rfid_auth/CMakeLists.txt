# cmake configuration file for the RFID Authentication Plugin

MYSQL_ADD_PLUGIN(rfid_auth rfid_auth.cc
  MODULE_ONLY MODULE_OUTPUT_NAME "rfid_auth")

INSTALL(FILES rfid_auth.ini DESTINATION ${INSTALL_PLUGINDIR})
