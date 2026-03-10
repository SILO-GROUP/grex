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
#include <gtk/gtk.h>
#include <vector>
#include "models/project.h"
#include "models/grex_config.h"

namespace grex {

enum class UnitDialogResult { Save, Cancel };

// Blocking modal dialog for editing unit properties.
// Edits a working copy; writes back to the original Unit on Save.
UnitDialogResult show_unit_properties_dialog(GtkWindow* parent,
    Unit* unit, Project& project, GrexConfig& grex_config,
    const std::vector<ShellDef>& shells);

}
