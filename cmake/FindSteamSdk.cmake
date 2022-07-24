# -----------------------------------------------------------------------------
# Find SteamSdk SDK
# Define:
# SteamSdk_FOUND
# SteamSdk_INCLUDE_DIR
# SteamSdk_LIBRARY
# SteamSdk_ROOT_DIR

set(SteamSdk_FOUND false)
message("Storm Extern Dir=${STORM_EXTERN_DIR}")

set(SteamSdk_INCLUDE_DIR "${STORM_EXTERN_DIR}/steam/inc")
#find_path(SteamSdk_INCLUDE_DIR steam_api.h "${Storm_EXTERN_DIR}/steam/inc")
mark_as_advanced(SteamSdk_INCLUDE_DIR)
if(SteamSdk_INCLUDE_DIR)
    set(SteamSdk_ROOT_DIR "${STORM_EXTERN_DIR}/steam")
endif()

set(SteamSdk_LIBRARY_PATHS "${SteamSdk_ROOT_DIR}/lib")

find_library(SteamSdk_LIBRARY steam_api ${SteamSdk_LIBRARY_PATHS} NO_DEFAULT_PATH)

if(SteamSdk_INCLUDE_DIR AND SteamSdk_LIBRARY)
    set(SteamSdk_FOUND true)
endif()
mark_as_advanced(SteamSdk_LIBRARY)

message("SteamSdk_FOUND=${SteamSdk_FOUND}")
message("SteamSdk_INCLUDE_DIR=${SteamSdk_INCLUDE_DIR}")
message("SteamSdk_LIBRARY=${SteamSdk_LIBRARY}")
message("SteamSdk_ROOT_DIR=${SteamSdk_ROOT_DIR}")
