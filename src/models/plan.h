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
#include <optional>
#include <filesystem>
#include <nlohmann/json.hpp>  // full include required: json used as member type

namespace grex {

struct Task {
    std::string name;
    nlohmann::json dependencies;  // JSON array preserved as-is for round-trip fidelity
    std::optional<std::string> comment;
};

void to_json(nlohmann::json& j, const Task& t);
void from_json(const nlohmann::json& j, Task& t);

struct Plan {
    std::string name;
    std::filesystem::path filepath;
    std::vector<Task> tasks;

    static Plan load(const std::filesystem::path& filepath);
    void save() const;
};

}
