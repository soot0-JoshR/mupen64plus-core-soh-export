cmake_minimum_required(VERSION 3.10)
project(soh_export C)

find_package(PkgConfig REQUIRED)
pkg_check_modules(CJSON cjson)

add_executable(soh_export soh_export.c)
target_compile_features(soh_export PRIVATE c_std_99)
target_include_directories(soh_export PRIVATE ${CJSON_INCLUDE_DIRS})
target_link_libraries(soh_export PRIVATE ${CJSON_LIBRARIES})
install(TARGETS soh_export DESTINATION bin)
install(FILES rom_mappings.json DESTINATION share/soh_export)
