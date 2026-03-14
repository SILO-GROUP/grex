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

#include "views/unit_editor.h"
#include "util/unit_picker.h"
#include "util/unit_properties_dialog.h"

namespace grex {

UnitEditor::UnitEditor(Project& project, GrexConfig& grex_config)
    : project_(project), grex_config_(grex_config) {
    root_ = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(root_), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

    content_box_ = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_margin_start(content_box_, 16);
    gtk_widget_set_margin_end(content_box_, 16);
    gtk_widget_set_margin_top(content_box_, 16);
    gtk_widget_set_margin_bottom(content_box_, 16);
    auto* box = content_box_;

    // Task properties container — disabled when no task selected
    task_section_ = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);

    auto* task_label = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(task_label), "<b>Task Properties</b>");
    gtk_label_set_xalign(GTK_LABEL(task_label), 0.0f);
    gtk_box_append(GTK_BOX(task_section_), task_label);

    auto* task_grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(task_grid), 6);
    gtk_grid_set_column_spacing(GTK_GRID(task_grid), 12);

    // Name (read-only label)
    auto* name_label = gtk_label_new("Name");
    gtk_label_set_xalign(GTK_LABEL(name_label), 1.0f);
    gtk_grid_attach(GTK_GRID(task_grid), name_label, 0, 0, 1, 1);
    name_display_ = gtk_label_new("");
    gtk_label_set_xalign(GTK_LABEL(name_display_), 0.0f);
    gtk_widget_set_hexpand(name_display_, TRUE);
    gtk_grid_attach(GTK_GRID(task_grid), name_display_, 1, 0, 1, 1);

    // Comment
    auto* comment_label = gtk_label_new("Comment");
    gtk_label_set_xalign(GTK_LABEL(comment_label), 1.0f);
    gtk_grid_attach(GTK_GRID(task_grid), comment_label, 0, 1, 1, 1);
    entry_comment_ = gtk_entry_new();
    gtk_widget_set_hexpand(entry_comment_, TRUE);
    gtk_grid_attach(GTK_GRID(task_grid), entry_comment_, 1, 1, 1, 1);

    gtk_box_append(GTK_BOX(task_section_), task_grid);
    gtk_box_append(GTK_BOX(box), task_section_);

    // Underlying Unit controls — exposed for external placement
    unit_controls_ = gtk_frame_new("Underlying Unit");
    auto* unit_btn_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_margin_start(unit_btn_box, 8);
    gtk_widget_set_margin_end(unit_btn_box, 8);
    gtk_widget_set_margin_top(unit_btn_box, 8);
    gtk_widget_set_margin_bottom(unit_btn_box, 8);

    btn_select_unit_ = gtk_button_new_with_label("Change/Select Unit...");
    g_signal_connect(btn_select_unit_, "clicked", G_CALLBACK(on_select_unit), this);
    gtk_box_append(GTK_BOX(unit_btn_box), btn_select_unit_);

    btn_edit_unit_ = gtk_button_new_with_label("Edit Unit...");
    g_signal_connect(btn_edit_unit_, "clicked", G_CALLBACK(on_edit_unit), this);
    gtk_box_append(GTK_BOX(unit_btn_box), btn_edit_unit_);

    btn_save_unit_ = gtk_button_new_with_label("Save Unit");
    g_signal_connect(btn_save_unit_, "clicked", G_CALLBACK(+[](GtkButton*, gpointer d) {
        auto* self = static_cast<UnitEditor*>(d);
        self->save_current();
    }), this);
    gtk_box_append(GTK_BOX(unit_btn_box), btn_save_unit_);

    gtk_frame_set_child(GTK_FRAME(unit_controls_), unit_btn_box);

    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(root_), box);

    clear();
}

void UnitEditor::mark_dirty() {
    dirty_ = true;
    gtk_widget_add_css_class(btn_save_unit_, "suggested-action");
}

void UnitEditor::clear_dirty() {
    dirty_ = false;
    gtk_widget_remove_css_class(btn_save_unit_, "suggested-action");
}

void UnitEditor::clear() {
    g_signal_handlers_disconnect_by_data(entry_comment_, this);
    current_task_ = nullptr;
    current_unit_ = nullptr;
    current_unit_name_.clear();

    gtk_label_set_text(GTK_LABEL(name_display_), "");
    gtk_editable_set_text(GTK_EDITABLE(entry_comment_), "");
    gtk_widget_set_sensitive(task_section_, FALSE);
    gtk_widget_set_sensitive(unit_controls_, FALSE);
    clear_dirty();
}

void UnitEditor::load(Task* task, Unit* unit) {
    gtk_widget_set_sensitive(task_section_, TRUE);
    gtk_widget_set_sensitive(unit_controls_, TRUE);
    g_signal_handlers_disconnect_by_data(entry_comment_, this);
    current_task_ = task;
    current_unit_ = unit;
    current_unit_name_ = task ? task->name : "";

    gtk_widget_set_sensitive(entry_comment_, TRUE);
    if (unit) {
        gtk_label_set_text(GTK_LABEL(name_display_), task->name.c_str());
    } else {
        auto markup = std::string("<span foreground=\"red\">") + task->name + "</span>";
        gtk_label_set_markup(GTK_LABEL(name_display_), markup.c_str());
    }
    gtk_editable_set_text(GTK_EDITABLE(entry_comment_), task->comment.value_or("").c_str());
    gtk_widget_set_sensitive(btn_edit_unit_, unit != nullptr);

    // Connect comment change signal
    g_signal_connect(entry_comment_, "changed", G_CALLBACK(+[](GtkEditable* e, gpointer d) {
        auto* self = static_cast<UnitEditor*>(d);
        if (!self->current_task_) return;
        self->mark_dirty();
        auto text = std::string(gtk_editable_get_text(e));
        self->current_task_->comment = text.empty() ? std::nullopt : std::optional<std::string>(text);
    }), this);

    clear_dirty();
}

void UnitEditor::save_current() {
    if (!current_unit_) {
        project_.report_status("Error: no unit loaded to save");
        return;
    }
    auto* uf = project_.find_unit_file(current_unit_->name);
    if (!uf) {
        project_.report_status("Error: cannot find unit file for '" + current_unit_->name + "'");
        return;
    }
    try {
        uf->save();
        clear_dirty();
        project_.report_status("Saved unit file: " + uf->filepath.filename().string());
    } catch (const std::exception& e) {
        project_.report_status(std::string("Error: ") + e.what());
    }
}

void UnitEditor::revert_current() {
    if (current_unit_name_.empty()) return;
    auto* uf = project_.find_unit_file(current_unit_name_);
    if (uf) {
        try {
            auto reloaded = UnitFile::load(uf->filepath);
            *uf = std::move(reloaded);
        } catch (const std::exception& e) {
            project_.report_status(std::string("Error: ") + e.what());
        }
    }
    if (!project_.plans.empty()) {
        auto& plan = project_.plans[0];
        try {
            auto reloaded = Plan::load(plan.filepath);
            plan = std::move(reloaded);
        } catch (const std::exception& e) {
            project_.report_status(std::string("Error: ") + e.what());
        }
    }
    clear_dirty();
}

void UnitEditor::set_name_changed_callback(NameChangedCallback cb, void* data) {
    name_cb_ = cb;
    name_cb_data_ = data;
}

void UnitEditor::on_edit_unit(GtkButton*, gpointer data) {
    auto* self = static_cast<UnitEditor*>(data);
    if (!self->current_unit_) return;

    auto* parent = GTK_WINDOW(gtk_widget_get_ancestor(self->root_, GTK_TYPE_WINDOW));
    auto result = show_unit_properties_dialog(parent, self->current_unit_,
        self->project_, self->grex_config_, self->project_.all_shells());

    if (result == UnitDialogResult::Save) {
        // Sync state in case the unit name changed in the dialog
        auto new_name = self->current_unit_->name;
        self->current_unit_name_ = new_name;
        if (self->current_task_)
            self->current_task_->name = new_name;
        gtk_label_set_text(GTK_LABEL(self->name_display_), new_name.c_str());
        if (self->name_cb_)
            self->name_cb_(new_name, self->name_cb_data_);
        self->mark_dirty();
    }
}

void UnitEditor::on_select_unit(GtkButton*, gpointer data) {
    auto* self = static_cast<UnitEditor*>(data);
    if (!self->current_task_) return;

    if (self->project_.unit_files.empty())
        self->project_.load_all_units();

    if (self->project_.unit_files.empty()) {
        self->project_.report_status("Error: no units loaded");
        return;
    }

    auto* parent = GTK_WINDOW(gtk_widget_get_ancestor(self->root_, GTK_TYPE_WINDOW));
    show_unit_picker(parent, self->project_, [self](const std::string& unit_name) {
        if (self->current_task_) self->current_task_->name = unit_name;
        Unit* unit = self->project_.find_unit(unit_name);
        self->load(self->current_task_, unit);
        if (self->name_cb_) self->name_cb_(unit_name, self->name_cb_data_);
    });
}

}
