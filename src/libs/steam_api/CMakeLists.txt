if (STORM_ENABLE_STEAM)
    set(steamworks steamworks)
endif()

if (WIN32)
STORM_SETUP(
    TARGET_NAME steam_api
    TYPE storm_module
    DEPENDENCIES common ${steamworks}
)
endif()