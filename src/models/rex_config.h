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

class RexConfig {
public:
    static RexConfig load(const std::filesystem::path& filepath);
    void save(const std::filesystem::path& filepath) const;

    // access the raw json "config" object
    nlohmann::json& data() { return data_; }
    const nlohmann::json& data() const { return data_; }

    // convenience: get a string value by key, empty string if missing
    std::string get(const std::string& key) const;
    void set(const std::string& key, const std::string& value);

    // return all string values (for variable scanning)
    std::vector<std::string> all_string_values() const;

private:
    nlohmann::json data_;
};

}
