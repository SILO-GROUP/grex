/*
        GREX - A graphical frontend for creating and managing Rex project files.
        Copyright (C) 2026  SILO GROUP, LLC.  Written by Chris Punches

        This program is free software: you can redistribute it and/or modify
        it under the terms of the GNU Affero General Public License as
        published by the Free Software Foundation, either version 3 of the
        License, or (at your option) any later version.

        This program is distributed in the hope that it will be useful,
        but WITHOUT ANY WARRANTY; without even the implied warranty of
        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
        GNU Affero General Public License for more details.

        You should have received a copy of the GNU Affero General Public License
        along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "util/json_helpers.h"
#include <fstream>
#include <stdexcept>

namespace grex {

nlohmann::json load_json_file(const std::filesystem::path& path) {
    std::ifstream f(path);
    if (!f.is_open())
        throw std::runtime_error("Cannot open file: " + path.string());
    return nlohmann::json::parse(f);
}

void save_json_file(const std::filesystem::path& path, const nlohmann::json& j) {
    std::ofstream f(path);
    if (!f.is_open())
        throw std::runtime_error("Cannot write file: " + path.string());
    f << j.dump(1, '\t') << '\n';
}

}
