find_package(Threads REQUIRED)

if(UNIX)
    find_package(DL REQUIRED)
endif()

include(FetchContent)

find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
message(STATUS "Linking with: ${LLVM_DEFINITIONS}")

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

llvm_map_components_to_libnames(llvm_libs core support mcparser option bitreader profiledata irreader executionengine mcjit native)
message(STATUS "Linking with: ${llvm_libs}")

# cxxopts
FetchContent_Declare(cxxopts
  GIT_REPOSITORY https://github.com/jarro2783/cxxopts.git
  GIT_TAG 5f43f4cbfee5d92560ece7811a2a44c763f9fb73)

FetchContent_GetProperties(cxxopts)
if(NOT cxxopts_POPULATED)
  FetchContent_Populate(cxxopts)
  add_subdirectory(${cxxopts_SOURCE_DIR} ${cxxopts_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()


if(WIN32)
    # winflexbision
    FetchContent_Declare(winflexbision
        GIT_REPOSITORY https://github.com/LonghronShen/winflexbison.git
        # GIT_REPOSITORY https://github.com/lexxmark/winflexbison.git
        GIT_TAG master)

    FetchContent_GetProperties(winflexbision)
    if(NOT winflexbision_POPULATED)
        FetchContent_Populate(winflexbision)
        add_subdirectory(${winflexbision_SOURCE_DIR} ${winflexbision_BINARY_DIR} EXCLUDE_FROM_ALL)

        execute_process(COMMAND ${CMAKE_COMMAND}
            -S ${winflexbision_SOURCE_DIR}
            -B ${CMAKE_BINARY_DIR}/external/winflexbision
            -G ${CMAKE_GENERATOR}
            -D CMAKE_BUILD_TYPE=Debug
            -D CMAKE_RUNTIME_OUTPUT_DIRECTORY=${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
            -D CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG=${CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG}
            -D CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE=${CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE}
        )

        execute_process(COMMAND ${CMAKE_COMMAND}
            --build ${CMAKE_BINARY_DIR}/external/winflexbision
        )

        execute_process(COMMAND ${CMAKE_COMMAND}
            --install ${CMAKE_BINARY_DIR}/external/winflexbision --prefix ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
        )

        set(BISON_ROOT_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}" CACHE STRING "BISON_ROOT_DIR" FORCE)
        set(FLEX_ROOT_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}" CACHE STRING "FLEX_ROOT_DIR" FORCE)

        set(BISON_EXECUTABLE "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/win_bison.exe" CACHE STRING "BISON_EXECUTABLE" FORCE)
        set(BISON_version_result "0" CACHE STRING "BISON_version_result" FORCE)
        set(BISON_version_output "bison++ Version 1,0,0" CACHE STRING "BISON_version_result" FORCE)

        set(FLEX_EXECUTABLE "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/win_flex.exe" CACHE STRING "FLEX_EXECUTABLE" FORCE)
        set(FLEX_version_result "0" CACHE STRING "FLEX_version_result" FORCE)
        set(FLEX_FIND_REQUIRED "0" CACHE STRING "FLEX_FIND_REQUIRED" FORCE)

        include(UseBISON)
        include(UseFLEX)
    endif()

    # unistd_h
    FetchContent_Declare(unistd_h
        GIT_REPOSITORY https://github.com/win32ports/unistd_h.git
        GIT_TAG 0dfc48c1bc67fa27b02478eefe0443b8d2750cc2)

    FetchContent_GetProperties(unistd_h)
    if(NOT unistd_h_POPULATED)
        FetchContent_Populate(unistd_h)
        # add_subdirectory(${unistd_h_SOURCE_DIR} ${unistd_h_BINARY_DIR} EXCLUDE_FROM_ALL)
        include_directories(${unistd_h_SOURCE_DIR})
    endif()
else()
    if(APPLE)
        find_program(MAC_HBREW_BIN brew)

        if(MAC_HBREW_BIN)
            execute_process(COMMAND ${MAC_HBREW_BIN} "--prefix" OUTPUT_VARIABLE BREW_PREFIX OUTPUT_STRIP_TRAILING_WHITESPACE)
            list(INSERT CMAKE_PREFIX_PATH 0 ${BREW_PREFIX})
        endif()

        execute_process(
            COMMAND ${MAC_HBREW_BIN} --prefix bison
            RESULT_VARIABLE BREW_BISON
            OUTPUT_VARIABLE BREW_BISON_PREFIX
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        if(BREW_BISON EQUAL 0 AND EXISTS "${BREW_BISON_PREFIX}")
            message(STATUS "Found Bison keg installed by Homebrew at ${BREW_BISON_PREFIX}")
            set(BISON_EXECUTABLE "${BREW_BISON_PREFIX}/bin/bison")
            list(INSERT CMAKE_PREFIX_PATH 0 "${BREW_BISON_PREFIX}")
        else()
            message(FATAL_ERROR "Cannot find bison from homebrew.")
        endif()

        execute_process(
            COMMAND ${MAC_HBREW_BIN} --prefix flex
            RESULT_VARIABLE BREW_FLEX
            OUTPUT_VARIABLE BREW_FLEX_PREFIX
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        if(BREW_FLEX EQUAL 0 AND EXISTS "${BREW_FLEX_PREFIX}")
            message(STATUS "Found Flex keg installed by Homebrew at ${BREW_FLEX_PREFIX}")
            set(FLEX_EXECUTABLE "${BREW_FLEX_PREFIX}/bin/flex")
            list(INSERT CMAKE_PREFIX_PATH 0 "${BREW_FLEX_PREFIX}")
        else()
            message(FATAL_ERROR "Cannot find flex from homebrew.")
        endif()
    endif()

    find_package(BISON REQUIRED)
    find_package(FLEX REQUIRED)
endif()