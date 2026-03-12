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

namespace grex {

class ConfigView {
public:
    explicit ConfigView(Project& project);
    GtkWidget* widget() { return root_; }

    void rebuild_variables();
    void update_resolved_labels();
    void apply_config();
    void refresh();
    bool is_dirty() const { return dirty_; }

    using ApplyCallback = void(*)(void* data);
    void set_apply_callback(ApplyCallback cb, void* data);

private:
    Project& project_;
    GtkWidget* root_;
    GtkWidget* config_grid_;
    GtkWidget* vars_grid_;
    GtkWidget* resolved_grid_;

    ApplyCallback apply_cb_ = nullptr;
    void* apply_cb_data_ = nullptr;
    bool rebuilding_ = false;
    bool dirty_ = false;

    std::vector<void*> field_bindings_;   // CfgFieldBinding*
    std::vector<void*> var_bindings_;     // CfgVarBinding*

    GtkWidget* config_label_;
    GtkWidget* btn_open_;
    GtkWidget* btn_create_;
    GtkWidget* btn_close_;
    GtkWidget* btn_save_;
    GtkWidget* config_content_;

    void build_config_fields();
    void build_variables_section();
    void update_config_buttons();

    static void on_open_config(GtkButton* btn, gpointer data);
    static void on_open_config_response(GObject* source, GAsyncResult* res, gpointer data);
    static void on_create_config(GtkButton* btn, gpointer data);
    static void on_create_config_response(GObject* source, GAsyncResult* res, gpointer data);
    static void on_close_config(GtkButton* btn, gpointer data);
};

}
