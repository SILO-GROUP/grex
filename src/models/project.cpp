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

#include "models/project.h"
#include <set>
#include <unistd.h>

namespace grex {
namespace fs = std::filesystem;

void Project::report_status(const std::string& msg) {
    if (status_cb)
        status_cb(msg, status_cb_data);
}

fs::path Project::resolved_project_root() const {
    auto val = config.get("project_root");
    if (val.empty() || !resolver.can_resolve(val))
        return {};
    return resolver.resolve(val);
}

fs::path Project::resolved_units_dir() const {
    auto root = resolved_project_root();
    if (root.empty()) return {};
    auto units = config.get("units_path");
    if (units.empty()) return {};
    return root / units;
}

fs::path Project::resolved_shells_path() const {
    auto root = resolved_project_root();
    if (root.empty()) return {};
    auto sp = config.get("shells_path");
    if (sp.empty()) return {};
    return root / sp;
}

bool Project::check_unit_valid(const Unit& u) {
    namespace fs = std::filesystem;

    if (u.target.empty()) {
        report_status("Unit '" + u.name + "': target is not defined");
        return false;
    }
    if (!fs::exists(u.target)) {
        report_status("Unit '" + u.name + "': target does not exist: " + u.target);
        return false;
    }
    if (access(u.target.c_str(), X_OK) != 0) {
        report_status("Unit '" + u.name + "': target is not executable: " + u.target);
        return false;
    }

    if (u.is_shell_command) {
        bool found = false;
        for (auto& s : all_shells()) {
            if (s.name == u.shell_definition) { found = true; break; }
        }
        if (!found) {
            report_status("Unit '" + u.name + "': shell definition not found: " + u.shell_definition);
            return false;
        }
    }

    if (u.set_working_directory) {
        if (u.working_directory.empty()) {
            report_status("Unit '" + u.name + "': working directory is not defined");
            return false;
        }
        if (!fs::is_directory(u.working_directory)) {
            report_status("Unit '" + u.name + "': working directory does not exist: " + u.working_directory);
            return false;
        }
    }

    if (u.rectify) {
        if (u.rectifier.empty()) {
            report_status("Unit '" + u.name + "': rectifier is not defined");
            return false;
        }
        if (!fs::exists(u.rectifier)) {
            report_status("Unit '" + u.name + "': rectifier does not exist: " + u.rectifier);
            return false;
        }
        if (access(u.rectifier.c_str(), X_OK) != 0) {
            report_status("Unit '" + u.name + "': rectifier is not executable: " + u.rectifier);
            return false;
        }
    }

    if (u.supply_environment) {
        if (u.environment.empty()) {
            report_status("Unit '" + u.name + "': environment file is not defined");
            return false;
        }
        if (!fs::exists(u.environment)) {
            report_status("Unit '" + u.name + "': environment file does not exist: " + u.environment);
            return false;
        }
    }

    return true;
}

Unit* Project::find_unit(const std::string& unit_name) {
    for (auto& uf : unit_files) {
        auto* u = uf.find_unit(unit_name);
        if (u) return u;
    }
    return nullptr;
}

UnitFile* Project::find_unit_file(const std::string& unit_name) {
    for (auto& uf : unit_files) {
        if (uf.find_unit(unit_name))
            return &uf;
    }
    return nullptr;
}

bool Project::is_unit_name_taken(const std::string& name, const Unit* exclude) const {
    for (auto& uf : unit_files) {
        for (auto& u : uf.units) {
            if (u.name == name && &u != exclude)
                return true;
        }
    }
    return false;
}

void Project::load_all_units() {
    auto u_dir = resolved_units_dir();
    if (u_dir.empty()) {
        report_status("Error: units path not resolved");
        return;
    }
    if (!fs::is_directory(u_dir)) {
        report_status("Error: units path is not a directory: " + u_dir.string());
        return;
    }

    std::vector<UnitFile> loaded;
    int total_units = 0;
    int file_count = 0;
    int duplicates = 0;
    std::set<std::string> seen_names;
    for (auto& entry : fs::directory_iterator(u_dir)) {
        if (entry.path().extension() == ".units") {
            try {
                auto uf = UnitFile::load(entry.path());
                // Remove units with duplicate names
                auto it = uf.units.begin();
                while (it != uf.units.end()) {
                    if (seen_names.count(it->name)) {
                        report_status("Warning: duplicate unit '" + it->name +
                            "' in " + entry.path().filename().string() + " — skipped");
                        it = uf.units.erase(it);
                        duplicates++;
                    } else {
                        seen_names.insert(it->name);
                        ++it;
                    }
                }
                total_units += (int)uf.units.size();
                file_count++;
                loaded.push_back(std::move(uf));
            } catch (const std::exception& e) {
                report_status("Error: failed to load " + entry.path().filename().string() + ": " + e.what());
            }
        }
    }
    unit_files = std::move(loaded);
    auto msg = "Loaded " + std::to_string(total_units) + " units from " +
               std::to_string(file_count) + " files at '" + u_dir.string() + "'";
    if (duplicates > 0)
        msg += " (" + std::to_string(duplicates) + " duplicates skipped)";
    report_status(msg);
}

Project Project::load(const fs::path& cfg_path) {
    Project proj;
    proj.config_path = fs::canonical(cfg_path);
    proj.config = RexConfig::load(proj.config_path);

    // scan all config string values for variables, populate from environment
    proj.resolver.scan_and_populate(proj.config.all_string_values());

    // auto-load all shells files from shells directory
    auto sp = proj.resolved_shells_path();
    if (!sp.empty() && fs::is_directory(sp)) {
        for (auto& entry : fs::directory_iterator(sp)) {
            if (entry.path().extension() == ".shells") {
                try {
                    proj.shell_files.push_back(ShellsFile::load(entry.path()));
                } catch (const std::exception&) {
                    // no status_cb set yet at load time
                }
            }
        }
    }

    // auto-load all units if path resolves
    proj.load_all_units();

    return proj;
}

void Project::load_plan(const fs::path& plan_path) {
    plans.clear();
    try {
        plans.push_back(Plan::load(plan_path));
        report_status("Loaded plan: " + plan_path.filename().string());
    } catch (const std::exception& e) {
        report_status("Error: failed to load plan: " + std::string(e.what()));
        throw;
    }
}

void Project::reload_shells() {
    auto sp = resolved_shells_path();
    if (sp.empty()) {
        report_status("Error: shells path not resolved");
        return;
    }
    if (!fs::is_directory(sp)) {
        report_status("Error: shells path is not a directory: " + sp.string());
        return;
    }

    shell_files.clear();
    int file_count = 0;
    int total_shells = 0;
    for (auto& entry : fs::directory_iterator(sp)) {
        if (entry.path().extension() == ".shells") {
            try {
                auto sf = ShellsFile::load(entry.path());
                total_shells += (int)sf.shells.size();
                file_count++;
                shell_files.push_back(std::move(sf));
            } catch (const std::exception& e) {
                report_status("Error loading shells: " + std::string(e.what()));
            }
        }
    }
    report_status("Loaded " + std::to_string(total_shells) + " shells from " +
                   std::to_string(file_count) + " files at '" + sp.string() + "'");
}

std::vector<ShellDef> Project::all_shells() const {
    std::vector<ShellDef> result;
    for (auto& sf : shell_files)
        result.insert(result.end(), sf.shells.begin(), sf.shells.end());
    return result;
}

void Project::save_config() {
    config.save(config_path);
    report_status("Saved config: " + config_path.filename().string());
}

void Project::save_plans() {
    for (auto& p : plans) {
        p.save();
        report_status("Saved plan: " + p.name);
    }
}

void Project::open_config(const fs::path& new_config_path) {
    config_path = fs::canonical(new_config_path);
    config = RexConfig::load(config_path);
    resolver = VarResolver();
    resolver.scan_and_populate(config.all_string_values());

    // reload shells
    reload_shells();

    // reload units
    load_all_units();

    // keep any loaded plans but note they may reference stale units now
    report_status("Opened config: " + config_path.filename().string());
}

void Project::close_config() {
    config = RexConfig();
    config_path.clear();
    resolver = VarResolver();
    plans.clear();
    unit_files.clear();
    shell_files.clear();
    report_status("Config closed");
}

}
