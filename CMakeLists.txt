#        GREX - A graphical frontend for creating and managing Rex project files.
#        Copyright (C) 2026  SILO GROUP, LLC.  Written by Chris Punches
#
#        This program is free software: you can redistribute it and/or modify
#        it under the terms of the GNU Affero General Public License as
#        published by the Free Software Foundation, either version 3 of the
#        License, or (at your option) any later version.
#
#        This program is distributed in the hope that it will be useful,
#        but WITHOUT ANY WARRANTY; without even the implied warranty of
#        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#        GNU Affero General Public License for more details.
#
#        You should have received a copy of the GNU Affero General Public License
#        along with this program.  If not, see <https://www.gnu.org/licenses/>.

project(grex LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK4 REQUIRED IMPORTED_TARGET gtk4)

file(GLOB_RECURSE GREX_SOURCES ${CMAKE_SOURCE_DIR}/src/*.cpp)

add_executable(grex ${GREX_SOURCES})

target_include_directories(grex PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/vendor
)

target_link_libraries(grex PRIVATE PkgConfig::GTK4)
