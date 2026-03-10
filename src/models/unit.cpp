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

#include "models/unit.h"
#include "util/json_helpers.h"

namespace grex {

void to_json(nlohmann::json& j, const Unit& u) {
    j = nlohmann::json{
        {"name", u.name},
        {"target", u.target},
        {"is_shell_command", u.is_shell_command},
        {"shell_definition", u.shell_definition},
        {"force_pty", u.force_pty},
        {"set_working_directory", u.set_working_directory},
        {"working_directory", u.working_directory},
        {"rectify", u.rectify},
        {"rectifier", u.rectifier},
        {"active", u.active},
        {"required", u.required},
        {"set_user_context", u.set_user_context},
        {"user", u.user},
        {"group", u.group},
        {"supply_environment", u.supply_environment},
        {"environment", u.environment}
    };
}

void from_json(const nlohmann::json& j, Unit& u) {
    j.at("name").get_to(u.name);
    j.at("target").get_to(u.target);
    j.at("is_shell_command").get_to(u.is_shell_command);
    j.at("shell_definition").get_to(u.shell_definition);
    j.at("force_pty").get_to(u.force_pty);
    j.at("set_working_directory").get_to(u.set_working_directory);
    j.at("working_directory").get_to(u.working_directory);
    j.at("rectify").get_to(u.rectify);
    j.at("rectifier").get_to(u.rectifier);
    j.at("active").get_to(u.active);
    j.at("required").get_to(u.required);
    j.at("set_user_context").get_to(u.set_user_context);
    j.at("user").get_to(u.user);
    j.at("group").get_to(u.group);
    j.at("supply_environment").get_to(u.supply_environment);
    j.at("environment").get_to(u.environment);
}

UnitFile UnitFile::load(const std::filesystem::path& filepath) {
    auto j = load_json_file(filepath);
    UnitFile uf;
    uf.name = filepath.stem().string();
    uf.filepath = filepath;
    uf.units = j.at("units").get<std::vector<Unit>>();
    return uf;
}

void UnitFile::save() const {
    nlohmann::json j;
    j["units"] = units;
    save_json_file(filepath, j);
}

Unit* UnitFile::find_unit(const std::string& unit_name) {
    for (auto& u : units)
        if (u.name == unit_name)
            return &u;
    return nullptr;
}

}
