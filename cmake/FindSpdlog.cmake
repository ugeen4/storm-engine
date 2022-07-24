# -----------------------------------------------------------------------------
# Find Spdlog
# Define:
# Spdlog_FOUND
# Spdlog_INCLUDE_DIR
# Spdlog_LIBRARY
# Spdlog_ROOT_DIR

set(Spdlog_FOUND false)
message("Storm Extern Dir=${STORM_EXTERN_DIR}")

set(Spdlog_INCLUDE_DIR "${STORM_EXTERN_DIR}/spdlog/inc")
#file(GLOB_RECURSE Spdlog_INCLUDE_DIR "${STORM_EXTERN_DIR}/spdlog/inc/spdlog/spdlog.h")
mark_as_advanced(Spdlog_INCLUDE_DIR)
if(Spdlog_INCLUDE_DIR)
  set(Spdlog_ROOT_DIR "${STORM_EXTERN_DIR}/spdlog")
endif()

set(Spdlog_LIBRARY_PATHS "${Spdlog_ROOT_DIR}/lib")

find_library(Spdlog_LIBRARY spdlog ${Spdlog_LIBRARY_PATHS} NO_DEFAULT_PATH)
  if(Spdlog_INCLUDE_DIR AND Spdlog_LIBRARY)
    set(Spdlog_FOUND true)
  endif()
mark_as_advanced(Spdlog_LIBRARY)

message("Spdlog_FOUND=${Spdlog_FOUND}")
message("Spdlog_INCLUDE_DIR=${Spdlog_INCLUDE_DIR}")
message("Spdlog_LIBRARY=${Spdlog_LIBRARY}")
message("Spdlog_ROOT_DIR=${Spdlog_ROOT_DIR}")
