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

    // auto-load shells if path resolves
    auto sp = proj.resolved_shells_path();
    if (!sp.empty() && fs::exists(sp)) {
        try {
            proj.shells = ShellsFile::load(sp);
        } catch (const std::exception& e) {
            // no status_cb set yet at load time — caller handles
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

void Project::load_shells(const fs::path& shells_path) {
    shells = ShellsFile();
    try {
        shells = ShellsFile::load(shells_path);
        report_status("Loaded shells: " + shells_path.filename().string());
    } catch (const std::exception& e) {
        report_status("Error: failed to load shells: " + std::string(e.what()));
        throw;
    }
}

void Project::reload_shells() {
    auto sp = resolved_shells_path();
    if (sp.empty()) {
        report_status("Error: shells path not resolved");
        return;
    }
    if (!fs::exists(sp)) {
        report_status("Error: shells file not found: " + sp.string());
        return;
    }
    try {
        shells = ShellsFile::load(sp);
        report_status("Loaded shells: " + sp.filename().string());
    } catch (const std::exception& e) {
        report_status("Error: failed to load shells: " + std::string(e.what()));
    }
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

void Project::save_shells() {
    if (shells.filepath.empty()) {
        report_status("Error: no shells loaded to save");
        return;
    }
    shells.save();
    report_status("Saved shells: " + shells.filepath.filename().string());
}

void Project::open_config(const fs::path& new_config_path) {
    config_path = fs::canonical(new_config_path);
    config = RexConfig::load(config_path);
    resolver = VarResolver();
    resolver.scan_and_populate(config.all_string_values());

    // reload shells
    shells = ShellsFile();
    auto sp = resolved_shells_path();
    if (!sp.empty() && fs::exists(sp)) {
        try {
            shells = ShellsFile::load(sp);
            report_status("Loaded shells: " + sp.filename().string());
        } catch (const std::exception& e) {
            report_status(std::string("Error loading shells: ") + e.what());
        }
    }

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
    shells = ShellsFile();
    report_status("Config closed");
}

}
