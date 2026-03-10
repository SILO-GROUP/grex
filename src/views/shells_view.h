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
#include "models/project.h"

namespace grex {

class ShellsView {
public:
    explicit ShellsView(Project& project);
    GtkWidget* widget() { return root_; }
    void refresh();

private:
    Project& project_;
    ShellsFile& shells_;
    GtkWidget* root_;
    GtkWidget* listbox_;

    GtkWidget* entry_name_;
    GtkWidget* entry_path_;
    GtkWidget* entry_exec_arg_;
    GtkWidget* entry_source_cmd_;
    GtkWidget* editor_grid_;

    int current_idx_ = -1;
    bool loading_ = false;

    void populate_list();
    void load_shell(int idx);
    void clear_editor();

    static void on_shell_selected(GtkListBox* box, GtkListBoxRow* row, gpointer data);
    static void on_add_shell(GtkButton* btn, gpointer data);
    static void on_delete_shell(GtkButton* btn, gpointer data);
};

}
