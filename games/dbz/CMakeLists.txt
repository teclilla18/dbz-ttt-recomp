# The game executable. Its generated C comes from tools/build_game.py and lives
# in build/gen (ignored); configure fails early with a clear message without it.
set(DBZ_GEN_DIR "${PROJECT_SOURCE_DIR}/build/gen" CACHE PATH "Generated C (tools/build_game.py)")
set(DBZ_MAIN_ELF "${PROJECT_SOURCE_DIR}/baserom/recon/EBOOT.elf" CACHE FILEPATH "Decrypted main ELF")
set(DBZ_ISO_PATH "${PROJECT_SOURCE_DIR}/baserom/game.iso" CACHE FILEPATH "The player's disc image copy")
set(DBZ_FS_ROOT "${PROJECT_SOURCE_DIR}/build/fs" CACHE PATH "Folder holding disc/ and ms/")

file(GLOB DBZ_GEN CONFIGURE_DEPENDS
    "${DBZ_GEN_DIR}/main/dbz_*.c"
    "${DBZ_GEN_DIR}/ovl*/ovl*_*.c"
    "${DBZ_GEN_DIR}/overlay_table.c")
if(NOT DBZ_GEN)
    message(FATAL_ERROR "No generated C in ${DBZ_GEN_DIR}. Run: python tools/build_game.py")
endif()

add_executable(dbz_ttt host.c overlays.c ${DBZ_GEN})
option(DBZ_TRACE "Record guest function entries and loop addresses for PC diagnosis" OFF)
if(DBZ_TRACE)
    target_compile_definitions(dbz_ttt PRIVATE PSPRECOMP_TRACE)
endif()
target_include_directories(dbz_ttt PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(dbz_ttt PRIVATE
    DBZ_MAIN_ELF="${DBZ_MAIN_ELF}" DBZ_FS_ROOT="${DBZ_FS_ROOT}" DBZ_ISO_PATH="${DBZ_ISO_PATH}")
target_link_libraries(dbz_ttt PRIVATE psprecomp)
if(MSVC)
    target_compile_options(dbz_ttt PRIVATE /MP /W1)
endif()
