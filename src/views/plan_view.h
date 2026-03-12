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

class UnitEditor;

class PlanView {
public:
    PlanView(Project& project, GrexConfig& grex_config);
    GtkWidget* widget() { return root_; }
    void refresh();
    bool is_dirty() const;
    void save_dirty();
    void revert_dirty();

private:
    Project& project_;
    GrexConfig& grex_config_;
    GtkWidget* root_;
    GtkWidget* plan_label_;
    GtkWidget* task_listbox_;
    GtkWidget* btn_open_plan_;
    GtkWidget* btn_create_plan_;
    GtkWidget* btn_close_plan_;
    GtkWidget* btn_save_plan_;
    GtkWidget* task_controls_;   // container for task list + buttons
    UnitEditor* unit_editor_;

    int current_task_idx_ = -1;
    bool plan_dirty_ = false;
    bool suppress_selection_ = false;

    void populate_task_list();
    void select_task(int idx);

    Plan* current_plan();

    static void on_open_plan(GtkButton* btn, gpointer data);
    static void on_open_plan_response(GObject* source, GAsyncResult* res, gpointer data);
    static void on_task_selected(GtkListBox* box, GtkListBoxRow* row, gpointer data);
    static void on_add_task(GtkButton* btn, gpointer data);
    static void on_delete_task(GtkButton* btn, gpointer data);
    static void on_move_up(GtkButton* btn, gpointer data);
    static void on_move_down(GtkButton* btn, gpointer data);
    static void on_close_plan(GtkButton* btn, gpointer data);
    static void on_create_plan(GtkButton* btn, gpointer data);
    static void on_create_plan_response(GObject* source, GAsyncResult* res, gpointer data);

    void refresh_task_row(int idx);
    void update_plan_buttons();
    void close_plan_impl();
};

}
