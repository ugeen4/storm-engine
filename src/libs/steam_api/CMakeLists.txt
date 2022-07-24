if (WIN32)
if (STORM_ENABLE_STEAM)
    set(steamworks steamworks)
endif()

STORM_SETUP(
    TARGET_NAME steam_api
    TYPE storm_module
    DEPENDENCIES common ${steamworks}
)
else()
if (STORM_ENABLE_STEAM)
    set(steamworks ${SteamSdk_LIBRARY})
endif()

STORM_SETUP(
    TARGET_NAME steam_api
    TYPE storm_module
    DEPENDENCIES common ${steamworks}
)
endif()