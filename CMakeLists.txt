
cmake_minimum_required(VERSION 3.15)

project(afb-perm-check-ext
	VERSION 0.0
	DESCRIPTION "Provide permission check API"
	HOMEPAGE_URL "https://github.com/redpesk-common/afb-perm-check-ext"
	LANGUAGES C
)

include(FindPkgConfig)
include(GNUInstallDirs)

pkg_check_modules(deps REQUIRED libafb>=5.5)

# Define target
add_library(afb-perm-check-ext SHARED src/perm-check.c)

target_include_directories(afb-perm-check-ext PRIVATE ${deps_INCLUDE_DIRS})
target_compile_options(afb-perm-check-ext PRIVATE ${deps_CFLAGS})
target_link_options(afb-perm-check-ext PRIVATE ${deps_LDFLAGS})
target_link_directories(afb-perm-check-ext PRIVATE ${deps_LIBRARY_DIRS})
target_link_libraries(afb-perm-check-ext PRIVATE ${deps_LIBRARIES})

install(TARGETS afb-perm-check-ext LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/afb-ext)


