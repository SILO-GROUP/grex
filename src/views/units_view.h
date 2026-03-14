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
#include "models/grex_config.h"

namespace grex {

class UnitsView {
public:
    UnitsView(Project& project, GrexConfig& grex_config);
    GtkWidget* widget() { return root_; }
    void refresh();
    bool is_dirty() const { return file_dirty_; }
    void save_current_file();

private:
    Project& project_;
    GrexConfig& grex_config_;
    GtkWidget* root_;
    GtkWidget* file_listbox_;
    GtkWidget* unit_listbox_;
    GtkWidget* file_label_;
    GtkWidget* btn_save_;

    UnitFile* selected_file_ = nullptr;
    bool file_dirty_ = false;
    bool suppress_selection_ = false;

    void populate_file_list();
    void populate_unit_list();
    GtkListBoxRow* find_file_row(UnitFile* uf);

    static void on_file_selected(GtkListBox* box, GtkListBoxRow* row, gpointer data);
    static void on_new_file(GtkButton* btn, gpointer data);
    static void on_new_file_response(GObject* source, GAsyncResult* res, gpointer data);
    static void on_delete_file(GtkButton* btn, gpointer data);
    static void on_new_unit(GtkButton* btn, gpointer data);
    static void on_delete_unit(GtkButton* btn, gpointer data);
    static void on_unit_activated(GtkListBox* box, GtkListBoxRow* row, gpointer data);
    static void on_edit_unit(GtkButton* btn, gpointer data);
    static void on_move_up(GtkButton* btn, gpointer data);
    static void on_move_down(GtkButton* btn, gpointer data);
};

}
