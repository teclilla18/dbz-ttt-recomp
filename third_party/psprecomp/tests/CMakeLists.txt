# Tests are self-contained: no game data, no ROMs, no disc images. Every input
# is either a hand-written instruction encoding or a synthetic buffer built in
# the test itself.

add_executable(test_decode test_decode.c)
target_link_libraries(test_decode PRIVATE allegrex_core)
add_test(NAME decode COMMAND test_decode)

add_executable(test_dispatch test_dispatch.c)
target_link_libraries(test_dispatch PRIVATE psprecomp)
add_test(NAME dispatch COMMAND test_dispatch)

add_executable(test_runtime test_runtime.c)
target_link_libraries(test_runtime PRIVATE psprecomp)
add_test(NAME runtime COMMAND test_runtime)

add_executable(test_container test_container.c)
target_link_libraries(test_container PRIVATE allegrex_core)
add_test(NAME container COMMAND test_container)

# Published standard vectors only (FIPS-197, SP 800-38A, RFC 4493) — no PSP
# key material, no game data.


add_executable(test_emit test_emit.c)
target_link_libraries(test_emit PRIVATE allegrex_core)
add_test(NAME emit COMMAND test_emit)

# Exercise emitted C as well as the decoder and runtime separately.
add_executable(gen_vcst gen_vcst.c)
target_link_libraries(gen_vcst PRIVATE allegrex_core)
set(vcst_dir "${CMAKE_CURRENT_BINARY_DIR}/vcst")
file(MAKE_DIRECTORY "${vcst_dir}")
add_custom_command(
    OUTPUT "${vcst_dir}/t_vcst_funcs_000.c" "${vcst_dir}/t_vcst_register.c"
           "${vcst_dir}/t_vcst_funcs.h" "${vcst_dir}/t_vcst_imports.c"
    COMMAND gen_vcst "${vcst_dir}"
    DEPENDS gen_vcst
    VERBATIM)
add_executable(test_emitted_vcst test_emitted_vcst.c
    "${vcst_dir}/t_vcst_funcs_000.c" "${vcst_dir}/t_vcst_register.c"
    "${vcst_dir}/t_vcst_imports.c")
target_link_libraries(test_emitted_vcst PRIVATE psprecomp)
add_test(NAME emitted_vcst COMMAND test_emitted_vcst)

add_executable(gen_vfpu_branches gen_vfpu_branches.c)
target_link_libraries(gen_vfpu_branches PRIVATE allegrex_core)
set(vbranch_dir "${CMAKE_CURRENT_BINARY_DIR}/vbranch")
file(MAKE_DIRECTORY "${vbranch_dir}")
add_custom_command(
    OUTPUT "${vbranch_dir}/t_vbranch_funcs_000.c" "${vbranch_dir}/t_vbranch_register.c"
           "${vbranch_dir}/t_vbranch_funcs.h" "${vbranch_dir}/t_vbranch_imports.c"
    COMMAND gen_vfpu_branches "${vbranch_dir}"
    DEPENDS gen_vfpu_branches
    VERBATIM)
add_executable(test_emitted_vfpu_branches test_emitted_vfpu_branches.c
    "${vbranch_dir}/t_vbranch_funcs_000.c" "${vbranch_dir}/t_vbranch_register.c"
    "${vbranch_dir}/t_vbranch_imports.c")
target_link_libraries(test_emitted_vfpu_branches PRIVATE psprecomp)
add_test(NAME emitted_vfpu_branches COMMAND test_emitted_vfpu_branches)
set_tests_properties(emitted_vcst emitted_vfpu_branches PROPERTIES TIMEOUT 10)

add_executable(test_hle test_hle.c)
target_link_libraries(test_hle PRIVATE psprecomp allegrex_core)
add_test(NAME hle COMMAND test_hle)

set(IO_OPEN_ROOT "${CMAKE_CURRENT_BINARY_DIR}/io-open-root")
file(MAKE_DIRECTORY "${IO_OPEN_ROOT}/disc")
file(WRITE "${IO_OPEN_ROOT}/disc/probe.txt" "ok")
add_executable(test_io_open test_io_open.c)
target_link_libraries(test_io_open PRIVATE psprecomp)
target_compile_definitions(test_io_open PRIVATE PSP_TEST_IO_ROOT="${IO_OPEN_ROOT}")
add_test(NAME io_open COMMAND test_io_open)

add_executable(test_scheduler test_scheduler.c)
target_link_libraries(test_scheduler PRIVATE psprecomp allegrex_core)
add_test(NAME scheduler COMMAND test_scheduler)
set_tests_properties(hle scheduler PROPERTIES TIMEOUT 60)

add_executable(test_thread_lifecycle test_thread_lifecycle.c)
target_link_libraries(test_thread_lifecycle PRIVATE psprecomp allegrex_core)
foreach(case delete pinned pending checkpoint0 checkpoint1 checkpoint2 checkpoint3 mask reset)
    add_test(NAME thread_${case} COMMAND test_thread_lifecycle ${case})
    set_tests_properties(thread_${case} PROPERTIES TIMEOUT 60)
endforeach()

add_executable(test_vfpu test_vfpu.c)
target_link_libraries(test_vfpu PRIVATE psprecomp)
add_test(NAME vfpu COMMAND test_vfpu)

add_executable(test_ctors test_ctors.c)
target_link_libraries(test_ctors PRIVATE psprecomp)
add_test(NAME ctors COMMAND test_ctors)

add_executable(test_raster test_raster.c)
target_link_libraries(test_raster PRIVATE psprecomp)
add_test(NAME raster COMMAND test_raster)

add_executable(test_ge_address test_ge_address.c)
target_link_libraries(test_ge_address PRIVATE psprecomp)
add_test(NAME ge_address COMMAND test_ge_address)
set_tests_properties(ge_address PROPERTIES TIMEOUT 10)

add_executable(test_guest_calls test_guest_calls.c)
target_link_libraries(test_guest_calls PRIVATE psprecomp)
add_test(NAME guest_calls COMMAND test_guest_calls)

add_executable(test_module test_module.c)
target_link_libraries(test_module PRIVATE psprecomp)
add_test(NAME module COMMAND test_module)

# Real host metadata, but only synthetic files: no game data.
set(io_stat_root "${CMAKE_CURRENT_BINARY_DIR}/io_stat_fixture")
file(MAKE_DIRECTORY "${io_stat_root}/disc/subdir")
file(WRITE "${io_stat_root}/disc/probe.txt" "ok")
add_executable(test_io_stat test_io_stat.c)
target_link_libraries(test_io_stat PRIVATE psprecomp)
target_compile_definitions(test_io_stat PRIVATE PSP_TEST_IO_ROOT="${io_stat_root}")
add_test(NAME io_stat COMMAND test_io_stat)
if(APPLE)
    # POSIX seek-and-write is sparse here. Windows needs explicit sparse-file
    # APIs, so its >4 GiB coverage is not registered or claimed by this test.
    add_test(NAME io_stat_large_file COMMAND test_io_stat --large-file)
else()
    message(STATUS "io_stat_large_file excluded: >4 GiB sparse fixture is macOS-only; no Windows large-file coverage")
endif()
add_executable(test_ctrl_latch test_ctrl_latch.c)
target_link_libraries(test_ctrl_latch PRIVATE psprecomp)
add_test(NAME ctrl_latch COMMAND test_ctrl_latch)
set_tests_properties(ctrl_latch PROPERTIES TIMEOUT 30)
# The hints file: a TOML subset written by the game's author, and the entries it
# forces into emission. Synthetic code only, like every other suite here.
add_executable(test_hints test_hints.c)
target_link_libraries(test_hints PRIVATE allegrex_core)
add_test(NAME hints COMMAND test_hints)
