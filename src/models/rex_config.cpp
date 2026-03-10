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

#include "models/rex_config.h"
#include "util/json_helpers.h"

namespace grex {

RexConfig RexConfig::load(const std::filesystem::path& filepath) {
    auto j = load_json_file(filepath);
    RexConfig c;
    c.data_ = j.at("config");
    return c;
}

void RexConfig::save(const std::filesystem::path& filepath) const {
    nlohmann::json j;
    j["config"] = data_;
    save_json_file(filepath, j);
}

std::string RexConfig::get(const std::string& key) const {
    if (data_.contains(key) && data_[key].is_string())
        return data_[key].get<std::string>();
    return {};
}

void RexConfig::set(const std::string& key, const std::string& value) {
    data_[key] = value;
}

std::vector<std::string> RexConfig::all_string_values() const {
    std::vector<std::string> vals;
    for (auto& [key, val] : data_.items()) {
        if (val.is_string())
            vals.push_back(val.get<std::string>());
    }
    return vals;
}

}
