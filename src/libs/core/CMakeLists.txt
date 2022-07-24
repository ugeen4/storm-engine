if (WIN32)
  set(CORE_DEP "diagnostics"
               "shared_headers"
               "steam_api" 
               "fast_float"
               ${SDL_LIBRARIES}
  )
else()
  set(CORE_DEP "diagnostics"
               "shared_headers"
               "steam_api"
               "fast_float"
               ${SDL_LIBRARIES}
  )
endif()


STORM_SETUP(
    TARGET_NAME core
    TYPE library
    DEPENDENCIES ${CORE_DEP}
)