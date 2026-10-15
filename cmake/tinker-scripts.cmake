# Large callback and data units exceed MSVC's default COFF section limit.
# Source options cover both the menu DLL and the optional test executable.
set_property(SOURCE
    "${SRC_DIR}/core/scripting/TinkerScript.cpp"
    "${SRC_DIR}/core/scripting/TinkerScriptData.cpp"
    APPEND PROPERTY COMPILE_OPTIONS "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/bigobj>"
)

# Avoid Clang's expensive O2 inlining pass over the large callback and table initializers.
# Keep scalar optimization without expanding every table and callback helper inline.
# All other menu sources keep the selected build settings.
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # Clang requires PCH optimization flags to match, so these two files use their own headers.
    set_source_files_properties(
        "${SRC_DIR}/core/scripting/TinkerScript.cpp"
        "${SRC_DIR}/core/scripting/TinkerScriptData.cpp"
        PROPERTIES SKIP_PRECOMPILE_HEADERS ON
    )
    if(CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
        set(TINKER_CLANG_OPTIMIZATION "/clang:-O1")
        set(TINKER_CLANG_INLINING "/clang:-fno-inline-functions")
        set(TINKER_CLANG_DEBUG_INFO "/clang:-gline-tables-only")
    else()
        set(TINKER_CLANG_OPTIMIZATION "-O1")
        set(TINKER_CLANG_INLINING "-fno-inline-functions")
        set(TINKER_CLANG_DEBUG_INFO "-gline-tables-only")
    endif()
    set_property(SOURCE
        "${SRC_DIR}/core/scripting/TinkerScript.cpp"
        "${SRC_DIR}/core/scripting/TinkerScriptData.cpp"
        APPEND PROPERTY COMPILE_OPTIONS
        "$<$<CONFIG:Release,RelWithDebInfo,MinSizeRel>:${TINKER_CLANG_OPTIMIZATION}>"
        "$<$<CONFIG:Release,RelWithDebInfo,MinSizeRel>:${TINKER_CLANG_INLINING}>"
        "$<$<CONFIG:RelWithDebInfo>:${TINKER_CLANG_DEBUG_INFO}>"
    )
    # Serialize the large units to avoid memory contention with their test copies.
    if(CMAKE_GENERATOR MATCHES "^Ninja" AND CMAKE_VERSION VERSION_GREATER_EQUAL "4.2")
        set_property(GLOBAL APPEND PROPERTY JOB_POOLS tinker_compile=1)
        set_source_files_properties(
            "${SRC_DIR}/core/scripting/TinkerScript.cpp"
            "${SRC_DIR}/core/scripting/TinkerScriptData.cpp"
            PROPERTIES JOB_POOL_COMPILE tinker_compile
        )
    endif()
endif()

# Make completion explicit in IDE logs after the linker exits successfully.
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E echo "Build successful: $<TARGET_FILE_NAME:${PROJECT_NAME}>"
    VERBATIM
)

# Keep game-free native checks separate from the injected menu DLL.
option(BUILD_SCRIPT_TESTS "Build the native TinkerScripts checks" OFF)
if(BUILD_SCRIPT_TESTS)
    add_executable(TinkerScriptssTests
        src/core/scripting/TinkerScriptssTests.cpp
        src/core/scripting/TinkerScript.cpp
        src/core/scripting/TinkerScriptData.cpp
        src/core/filemgr/BaseObj.cpp
        src/core/filemgr/File.cpp
        src/core/filemgr/FileMgr.cpp
        src/core/filemgr/Folder.cpp
    )
    set_property(TARGET TinkerScriptssTests PROPERTY CXX_STANDARD 23)
    target_compile_definitions(TinkerScriptssTests PRIVATE TINKER_SCRIPT_TESTING)
    get_target_property(MENU_INCLUDE_DIRS ${PROJECT_NAME} INCLUDE_DIRECTORIES)
    target_include_directories(TinkerScriptssTests PRIVATE ${MENU_INCLUDE_DIRS})
    target_precompile_headers(TinkerScriptssTests PRIVATE "${SRC_DIR}/common.hpp")
    target_link_libraries(TinkerScriptssTests PRIVATE AsyncLogger imgui nlohmann_json::nlohmann_json)
endif()
