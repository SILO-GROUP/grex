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

#include "models/grex_config.h"
#include <fstream>
#include <cstdlib>

namespace grex {
namespace fs = std::filesystem;

fs::path GrexConfig::config_path() {
    const char* xdg = std::getenv("XDG_CONFIG_HOME");
    fs::path dir;
    if (xdg && xdg[0] != '\0') {
        dir = fs::path(xdg) / "grex";
    } else {
        const char* home = std::getenv("HOME");
        dir = fs::path(home ? home : "/tmp") / ".config" / "grex";
    }
    return dir / "grex.ini";
}

GrexConfig GrexConfig::load() {
    GrexConfig cfg;
    cfg.filepath_ = config_path();

    if (!fs::exists(cfg.filepath_)) {
        fs::create_directories(cfg.filepath_.parent_path());
        cfg.file_editor = "xterm -e vim";
        cfg.save();
        return cfg;
    }

    std::ifstream in(cfg.filepath_);
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#' || line[0] == ';')
            continue;
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        auto key = line.substr(0, eq);
        auto val = line.substr(eq + 1);
        // trim whitespace
        while (!key.empty() && key.back() == ' ') key.pop_back();
        while (!val.empty() && val.front() == ' ') val.erase(val.begin());

        if (key == "file_editor") cfg.file_editor = val;
    }

    return cfg;
}

void GrexConfig::save() const {
    fs::create_directories(filepath_.parent_path());
    std::ofstream out(filepath_);
    out << "file_editor=" << file_editor << "\n";
}

}
