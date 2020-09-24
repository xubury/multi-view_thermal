externalproject_add(ext_mve
    PREFIX          mve
    GIT_REPOSITORY  https://github.com/simonfuhrmann/mve.git
    UPDATE_COMMAND  ""
    SOURCE_DIR      ${CMAKE_SOURCE_DIR}/elibs/mve
    CONFIGURE_COMMAND ""
    BUILD_COMMAND   ${CMAKE_MAKE_PROGRAM} -C libs/mve && ${CMAKE_MAKE_PROGRAM} -C libs/util #not platform independent
    BUILD_IN_SOURCE 1
    INSTALL_COMMAND ""
)