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

#include "util/var_resolver.h"
#include <cstdlib>
#include <regex>

namespace grex {

std::set<std::string> VarResolver::find_variables(const std::string& input) {
    std::set<std::string> vars;
    // match ${var_name} patterns
    std::regex re(R"(\$\{([a-zA-Z_][a-zA-Z0-9_]*)\})");
    auto begin = std::sregex_iterator(input.begin(), input.end(), re);
    auto end = std::sregex_iterator();
    for (auto it = begin; it != end; ++it)
        vars.insert((*it)[1].str());
    return vars;
}

void VarResolver::set(const std::string& name, const std::string& value) {
    overrides_[name] = value;
}

std::string VarResolver::resolve(const std::string& input) const {
    std::string result = input;
    std::regex re(R"(\$\{([a-zA-Z_][a-zA-Z0-9_]*)\})");
    std::string output;
    auto begin = std::sregex_iterator(result.begin(), result.end(), re);
    auto end = std::sregex_iterator();

    size_t last_pos = 0;
    for (auto it = begin; it != end; ++it) {
        output.append(result, last_pos, it->position() - last_pos);
        auto var_name = (*it)[1].str();

        // check overrides first
        auto ov = overrides_.find(var_name);
        if (ov != overrides_.end() && !ov->second.empty()) {
            output.append(ov->second);
        } else {
            // check process environment
            const char* env_val = std::getenv(var_name.c_str());
            if (env_val && env_val[0] != '\0') {
                output.append(env_val);
            } else {
                // leave unresolved
                output.append(it->str());
            }
        }
        last_pos = it->position() + it->length();
    }
    output.append(result, last_pos);
    return output;
}

bool VarResolver::can_resolve(const std::string& input) const {
    auto vars = find_variables(input);
    for (auto& v : vars) {
        auto ov = overrides_.find(v);
        if (ov != overrides_.end() && !ov->second.empty())
            continue;
        const char* env = std::getenv(v.c_str());
        if (env && env[0] != '\0')
            continue;
        return false;
    }
    return true;
}

void VarResolver::scan_and_populate(const std::vector<std::string>& inputs) {
    for (auto& input : inputs) {
        auto vars = find_variables(input);
        for (auto& v : vars) {
            if (overrides_.find(v) != overrides_.end())
                continue;
            const char* env_val = std::getenv(v.c_str());
            overrides_[v] = env_val ? env_val : "";
        }
    }
}

std::vector<std::string> VarResolver::unresolved() const {
    std::vector<std::string> result;
    for (auto& [name, value] : overrides_) {
        if (value.empty() && !std::getenv(name.c_str()))
            result.push_back(name);
    }
    return result;
}

}
