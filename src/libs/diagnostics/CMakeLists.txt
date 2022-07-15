STORM_SETUP(
    TARGET_NAME diagnostics
    TYPE library
    DEPENDENCIES common core util ${Spdlog_LIBRARY}
)