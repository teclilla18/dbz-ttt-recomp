# Dragon Ball Z: Tenkaichi Tag Team static recompilation.
# Builds the vendored psprecomp toolkit (MIT, third_party/psprecomp) and the
# game from C generated locally by tools/build_game.py. See README.md.
cmake_minimum_required(VERSION 3.16)
project(dbz_ttt_recomp C)

# The game waits on semaphores other guest threads signal: real threads.
set(PSPRECOMP_REAL_THREADS ON CACHE BOOL "DBZ needs real guest threads" FORCE)
add_subdirectory(third_party/psprecomp)
add_subdirectory(games/dbz)
