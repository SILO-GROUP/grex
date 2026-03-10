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
#include <map>
#include <set>
#include <vector>

namespace grex {

class VarResolver {
public:
    // scan a string for ${var} patterns, return variable names found
    static std::set<std::string> find_variables(const std::string& input);

    // set a variable override (from UI)
    void set(const std::string& name, const std::string& value);

    // resolve all ${var} patterns in a string using:
    // 1. user overrides (set via UI)
    // 2. process environment (getenv)
    // returns the string with variables expanded; unresolvable vars left as-is
    std::string resolve(const std::string& input) const;

    // check if all variables in a string can be resolved
    bool can_resolve(const std::string& input) const;

    // get all known variables and their values (empty string if unresolved)
    const std::map<std::string, std::string>& overrides() const { return overrides_; }

    // scan multiple strings, populate overrides_ with env values where available
    void scan_and_populate(const std::vector<std::string>& inputs);

    // get list of variable names that have no value
    std::vector<std::string> unresolved() const;

private:
    std::map<std::string, std::string> overrides_;
};

}
