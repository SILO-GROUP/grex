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

#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include <nlohmann/json.hpp>  // full include required: json used in to_json/from_json signatures

namespace grex {

struct ShellDef {
    std::string name;
    std::string path;
    std::string execution_arg;
    std::string source_cmd;
};

void to_json(nlohmann::json& j, const ShellDef& s);
void from_json(const nlohmann::json& j, ShellDef& s);

struct ShellsFile {
    std::filesystem::path filepath;
    std::vector<ShellDef> shells;

    static ShellsFile load(const std::filesystem::path& filepath);
    void save() const;
};

}
