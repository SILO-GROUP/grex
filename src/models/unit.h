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
#include <nlohmann/json.hpp>  // full include required: json used as member type

namespace grex {

struct Unit {
    std::string name;
    std::string target;
    bool is_shell_command = true;
    std::string shell_definition = "bash";
    bool force_pty = false;
    bool set_working_directory = false;
    std::string working_directory;
    bool rectify = false;
    std::string rectifier;
    bool active = true;
    bool required = true;
    bool set_user_context = false;
    std::string user;
    std::string group;
    bool supply_environment = false;
    std::string environment;
};

void to_json(nlohmann::json& j, const Unit& u);
void from_json(const nlohmann::json& j, Unit& u);

struct UnitFile {
    std::string name;
    std::filesystem::path filepath;
    std::vector<Unit> units;

    static UnitFile load(const std::filesystem::path& filepath);
    void save() const;
    Unit* find_unit(const std::string& unit_name);
};

}
