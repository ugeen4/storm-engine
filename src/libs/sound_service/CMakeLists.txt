STORM_SETUP(
    TARGET_NAME sound_service
    TYPE storm_module
    DEPENDENCIES common core ${OGGVORBIS_LIBRARIES} ${ALUT_LIBRARIES}
)