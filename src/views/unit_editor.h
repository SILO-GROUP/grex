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
#include <string>
#include "models/project.h"
#include "models/grex_config.h"

namespace grex {

class UnitEditor {
public:
    UnitEditor(Project& project, GrexConfig& grex_config);
    ~UnitEditor() = default;
    GtkWidget* widget() { return root_; }
    GtkWidget* content_box() { return content_box_; }
    GtkWidget* unit_controls() { return unit_controls_; }

    void load(Task* task, Unit* unit);
    void clear();

    bool is_dirty() const { return dirty_; }
    void mark_dirty();
    void clear_dirty();
    void save_current();
    void revert_current();

    using UnitEditedCallback = void(*)(const std::string& new_name, bool name_changed, void* data);
    void set_unit_edited_callback(UnitEditedCallback cb, void* data);

private:
    Project& project_;
    GrexConfig& grex_config_;
    Task* current_task_ = nullptr;
    Unit* current_unit_ = nullptr;
    std::string current_unit_name_;
    GtkWidget* root_;
    GtkWidget* content_box_;
    GtkWidget* unit_controls_;
    GtkWidget* task_section_;

    // task fields
    GtkWidget* name_display_;
    GtkWidget* btn_select_unit_;
    GtkWidget* btn_edit_unit_;
    GtkWidget* btn_save_unit_;
    GtkWidget* entry_comment_;

    UnitEditedCallback edit_cb_ = nullptr;
    void* edit_cb_data_ = nullptr;

    bool dirty_ = false;

    static void on_select_unit(GtkButton* btn, gpointer data);
    static void on_edit_unit(GtkButton* btn, gpointer data);
};

}
