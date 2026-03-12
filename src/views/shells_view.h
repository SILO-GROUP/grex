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
    GtkWidget* root_;

    // Left panel: shell files
    GtkWidget* file_listbox_;
    GtkWidget* btn_save_;

    // Right panel: shells in selected file
    GtkWidget* file_label_;
    GtkWidget* shell_listbox_;

    // Editor
    GtkWidget* editor_grid_;
    GtkWidget* entry_name_;
    GtkWidget* entry_path_;
    GtkWidget* entry_exec_arg_;
    GtkWidget* entry_source_cmd_;

    int current_file_idx_ = -1;
    int current_shell_idx_ = -1;
    bool loading_ = false;
    bool file_dirty_ = false;

    void populate_file_list();
    void populate_shell_list();
    void load_shell(int idx);
    void clear_editor();
    void mark_file_dirty();

    static void on_file_selected(GtkListBox* box, GtkListBoxRow* row, gpointer data);
    static void on_new_file(GtkButton* btn, gpointer data);
    static void on_new_file_response(GObject* source, GAsyncResult* res, gpointer data);
    static void on_delete_file(GtkButton* btn, gpointer data);
    static void on_new_shell(GtkButton* btn, gpointer data);
    static void on_delete_shell(GtkButton* btn, gpointer data);
    static void on_move_up(GtkButton* btn, gpointer data);
    static void on_move_down(GtkButton* btn, gpointer data);
};

}
