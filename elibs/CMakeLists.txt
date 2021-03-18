ExternalProject_Add(
    ext_mve
    PREFIX mve
    GIT_REPOSITORY https://github.com/xubury/mve.git
    UPDATE_COMMAND ""
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/elibs/mve
    CONFIGURE_COMMAND  ${CMAKE_MAKE_PROGRAM} -C libs/mve && ${CMAKE_MAKE_PROGRAM} -C libs/util
    && ${CMAKE_MAKE_PROGRAM} -C libs/sfm && ${CMAKE_MAKE_PROGRAM} -C
    libs/dmrecon && ${CMAKE_MAKE_PROGRAM} -C libs/fssr
    BUILD_COMMAND
        ${CMAKE_MAKE_PROGRAM} -C libs/mve && ${CMAKE_MAKE_PROGRAM} -C libs/util
        && ${CMAKE_MAKE_PROGRAM} -C libs/sfm && ${CMAKE_MAKE_PROGRAM} -C
        libs/dmrecon && ${CMAKE_MAKE_PROGRAM} -C libs/fssr
    BUILD_IN_SOURCE 1
    INSTALL_COMMAND "")

ExternalProject_Add(
    ext_smvs
    PREFIX smvs
    GIT_REPOSITORY https://github.com/xubury/smvs
    UPDATE_COMMAND ""
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/elibs/smvs
    CONFIGURE_COMMAND ${CMAKE_MAKE_PROGRAM} -C lib
    BUILD_COMMAND ${CMAKE_MAKE_PROGRAM} -C lib
    BUILD_IN_SOURCE 1
    INSTALL_COMMAND "")
