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
#include <filesystem>
#include <vector>
#include "models/rex_config.h"
#include "models/plan.h"
#include "models/unit.h"
#include "models/shell_def.h"
#include "util/var_resolver.h"

namespace grex {

class Project {
public:
    static Project load(const std::filesystem::path& config_path);

    RexConfig config;
    std::filesystem::path config_path;
    VarResolver resolver;

    std::vector<Plan> plans;
    std::vector<UnitFile> unit_files;
    std::vector<ShellsFile> shell_files;

    // status reporting
    using StatusCallback = void(*)(const std::string& msg, void* data);
    StatusCallback status_cb = nullptr;
    void* status_cb_data = nullptr;
    void report_status(const std::string& msg);

    // resolved paths — empty if variables can't be resolved
    std::filesystem::path resolved_project_root() const;
    std::filesystem::path resolved_units_dir() const;
    std::filesystem::path resolved_shells_path() const;

    bool check_unit_valid(const Unit& u);
    Unit* find_unit(const std::string& unit_name);
    UnitFile* find_unit_file(const std::string& unit_name);
    bool is_unit_name_taken(const std::string& name, const Unit* exclude = nullptr) const;
    void load_all_units();
    void load_plan(const std::filesystem::path& plan_path);
    void reload_shells();
    std::vector<ShellDef> all_shells() const;
    void save_config();
    void save_plans();

    void open_config(const std::filesystem::path& new_config_path);
    void close_config();
};

}
