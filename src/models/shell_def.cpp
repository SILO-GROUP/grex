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

#include "models/shell_def.h"
#include "util/json_helpers.h"

namespace grex {

void to_json(nlohmann::json& j, const ShellDef& s) {
    j = nlohmann::json{
        {"name", s.name},
        {"path", s.path},
        {"execution_arg", s.execution_arg},
        {"source_cmd", s.source_cmd}
    };
}

void from_json(const nlohmann::json& j, ShellDef& s) {
    j.at("name").get_to(s.name);
    j.at("path").get_to(s.path);
    j.at("execution_arg").get_to(s.execution_arg);
    j.at("source_cmd").get_to(s.source_cmd);
}

ShellsFile ShellsFile::load(const std::filesystem::path& filepath) {
    auto j = load_json_file(filepath);
    ShellsFile sf;
    sf.filepath = filepath;
    sf.shells = j.at("shells").get<std::vector<ShellDef>>();
    return sf;
}

void ShellsFile::save() const {
    nlohmann::json j;
    j["shells"] = shells;
    save_json_file(filepath, j);
}

}
