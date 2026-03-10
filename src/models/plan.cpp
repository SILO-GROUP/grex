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

#include "models/plan.h"
#include "util/json_helpers.h"

namespace grex {

void to_json(nlohmann::json& j, const Task& t) {
    j = nlohmann::json{
        {"name", t.name},
        {"dependencies", t.dependencies}
    };
    if (t.comment.has_value())
        j["comment"] = t.comment.value();
}

void from_json(const nlohmann::json& j, Task& t) {
    j.at("name").get_to(t.name);
    j.at("dependencies").get_to(t.dependencies);
    if (j.contains("comment"))
        t.comment = j.at("comment").get<std::string>();
}

Plan Plan::load(const std::filesystem::path& filepath) {
    auto j = load_json_file(filepath);
    Plan p;
    p.name = filepath.stem().string();
    p.filepath = filepath;
    p.tasks = j.at("plan").get<std::vector<Task>>();
    return p;
}

void Plan::save() const {
    nlohmann::json j;
    j["plan"] = tasks;
    save_json_file(filepath, j);
}

}
