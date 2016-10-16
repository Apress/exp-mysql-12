# Spartan storage engine plugin

SET(SPARTAN_PLUGIN_STATIC "spartan")
SET(SPARTAN_PLUGIN_DYNAMIC "spartan")

SET(SPARTAN_SOURCES
   ha_spartan.cc ha_spartan.h
   spartan_data.cc spartan_data.h
   spartan_index.cc spartan_index.h
)

MYSQL_ADD_PLUGIN(spartan ${SPARTAN_SOURCES} STORAGE_ENGINE MODULE_ONLY)

TARGET_LINK_LIBRARIES(spartan mysys)
