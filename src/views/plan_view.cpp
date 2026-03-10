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

#include "views/plan_view.h"
#include "views/unit_editor.h"
#include "util/unsaved_dialog.h"
#include "util/unit_picker.h"
#include <algorithm>
#include <cstring>

namespace grex {

PlanView::PlanView(Project& project, GrexConfig& grex_config)
    : project_(project), grex_config_(grex_config) {
    root_ = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_paned_set_position(GTK_PANED(root_), 300);

    // === Left panel ===
    auto* left = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_widget_set_size_request(left, 200, -1);

    // Plan label
    plan_label_ = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(plan_label_), "<b>Plan:</b> No plan loaded");
    gtk_label_set_xalign(GTK_LABEL(plan_label_), 0.0f);
    gtk_widget_set_margin_start(plan_label_, 4);
    gtk_widget_set_margin_end(plan_label_, 4);
    gtk_widget_set_margin_top(plan_label_, 4);
    gtk_widget_add_css_class(plan_label_, "title-3");
    gtk_box_append(GTK_BOX(left), plan_label_);

    // Plan management buttons (Open/Create shown when no plan, Close shown when plan loaded)
    auto* mgmt_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_widget_set_margin_start(mgmt_row, 4);
    gtk_widget_set_margin_end(mgmt_row, 4);

    btn_open_plan_ = gtk_button_new_with_label("Open Plan");
    btn_create_plan_ = gtk_button_new_with_label("Create Plan");
    btn_close_plan_ = gtk_button_new_with_label("Close Plan");
    gtk_widget_set_hexpand(btn_open_plan_, TRUE);
    gtk_widget_set_hexpand(btn_create_plan_, TRUE);
    gtk_widget_set_hexpand(btn_close_plan_, TRUE);
    gtk_box_append(GTK_BOX(mgmt_row), btn_open_plan_);
    gtk_box_append(GTK_BOX(mgmt_row), btn_create_plan_);
    gtk_box_append(GTK_BOX(mgmt_row), btn_close_plan_);
    gtk_box_append(GTK_BOX(left), mgmt_row);

    // Task list
    auto* scroll = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_vexpand(scroll, TRUE);
    task_listbox_ = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(task_listbox_), GTK_SELECTION_SINGLE);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), task_listbox_);
    gtk_box_append(GTK_BOX(left), scroll);

    // Buttons
    auto* btn_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_widget_set_margin_start(btn_box, 4);
    gtk_widget_set_margin_end(btn_box, 4);
    gtk_widget_set_margin_bottom(btn_box, 4);

    auto* btn_add = gtk_button_new_with_label("Add Task");
    auto* btn_del = gtk_button_new_with_label("Delete Task");
    auto* btn_up = gtk_button_new_with_label("Up");
    auto* btn_down = gtk_button_new_with_label("Down");
    gtk_box_append(GTK_BOX(btn_box), btn_add);
    gtk_box_append(GTK_BOX(btn_box), btn_del);
    gtk_box_append(GTK_BOX(btn_box), btn_up);
    gtk_box_append(GTK_BOX(btn_box), btn_down);
    gtk_box_append(GTK_BOX(left), btn_box);

    // Save Plan button
    btn_save_plan_ = gtk_button_new_with_label("Save Plan");
    gtk_widget_set_margin_start(btn_save_plan_, 4);
    gtk_widget_set_margin_end(btn_save_plan_, 4);
    gtk_widget_set_margin_bottom(btn_save_plan_, 4);
    gtk_widget_set_hexpand(btn_save_plan_, TRUE);
    gtk_box_append(GTK_BOX(left), btn_save_plan_);

    gtk_paned_set_start_child(GTK_PANED(root_), left);
    gtk_paned_set_shrink_start_child(GTK_PANED(root_), FALSE);

    // === Right panel: Unit editor ===
    unit_editor_ = new UnitEditor(project_, grex_config_);
    gtk_paned_set_end_child(GTK_PANED(root_), unit_editor_->widget());
    gtk_paned_set_shrink_end_child(GTK_PANED(root_), FALSE);

    // Name change callback to refresh the task list row label
    unit_editor_->set_name_changed_callback([](const std::string&, void* data) {
        auto* self = static_cast<PlanView*>(data);
        self->plan_dirty_ = true;
        gtk_widget_add_css_class(self->btn_save_plan_, "suggested-action");
        if (self->current_task_idx_ >= 0)
            self->refresh_task_row(self->current_task_idx_);
    }, this);

    // Signals
    g_signal_connect(btn_open_plan_, "clicked", G_CALLBACK(on_open_plan), this);
    g_signal_connect(btn_create_plan_, "clicked", G_CALLBACK(on_create_plan), this);
    g_signal_connect(btn_close_plan_, "clicked", G_CALLBACK(on_close_plan), this);
    g_signal_connect(task_listbox_, "row-selected", G_CALLBACK(on_task_selected), this);
    g_signal_connect(btn_add, "clicked", G_CALLBACK(on_add_task), this);
    g_signal_connect(btn_del, "clicked", G_CALLBACK(on_delete_task), this);
    g_signal_connect(btn_up, "clicked", G_CALLBACK(on_move_up), this);
    g_signal_connect(btn_down, "clicked", G_CALLBACK(on_move_down), this);

    g_signal_connect(btn_save_plan_, "clicked", G_CALLBACK(+[](GtkButton*, gpointer d) {
        auto* self = static_cast<PlanView*>(d);
        try {
            self->project_.save_plans();
            self->plan_dirty_ = false;
            gtk_widget_remove_css_class(self->btn_save_plan_, "suggested-action");
        } catch (const std::exception& e) {
            self->project_.report_status(std::string("Error: ") + e.what());
        }
    }), this);

    update_plan_buttons();

}

Plan* PlanView::current_plan() {
    if (project_.plans.empty())
        return nullptr;
    return &project_.plans[0];
}

void PlanView::populate_task_list() {
    GtkWidget* child;
    while ((child = gtk_widget_get_first_child(task_listbox_)) != nullptr)
        gtk_list_box_remove(GTK_LIST_BOX(task_listbox_), child);

    unit_editor_->clear();
    current_task_idx_ = -1;

    auto* plan = current_plan();
    if (!plan) return;

    for (auto& task : plan->tasks) {
        auto* row = gtk_list_box_row_new();
        auto text = std::string("\u25B6 ") + task.name;
        auto* label = gtk_label_new(text.c_str());
        gtk_label_set_xalign(GTK_LABEL(label), 0.0f);
        gtk_widget_set_margin_start(label, 8);
        gtk_widget_set_margin_end(label, 8);
        gtk_widget_set_margin_top(label, 4);
        gtk_widget_set_margin_bottom(label, 4);
        gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), label);
        gtk_list_box_append(GTK_LIST_BOX(task_listbox_), row);
    }
}

void PlanView::refresh_task_row(int idx) {
    auto* plan = current_plan();
    if (!plan || idx < 0 || idx >= (int)plan->tasks.size()) return;

    auto* row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(task_listbox_), idx);
    if (!row) return;

    auto* label = gtk_list_box_row_get_child(GTK_LIST_BOX_ROW(row));
    if (GTK_IS_LABEL(label)) {
        auto text = std::string("\u25B6 ") + plan->tasks[idx].name;
        gtk_label_set_text(GTK_LABEL(label), text.c_str());
    }
}

void PlanView::select_task(int idx) {
    auto* plan = current_plan();
    if (!plan || idx < 0 || idx >= (int)plan->tasks.size()) {
        unit_editor_->clear();
        return;
    }

    current_task_idx_ = idx;
    auto& task = plan->tasks[idx];

    // ensure units are loaded if paths resolve
    if (project_.unit_files.empty())
        project_.load_all_units();

    Unit* unit = project_.find_unit(task.name);

    unit_editor_->load(&task, unit);
}

// --- Open Plan ---

void PlanView::on_open_plan(GtkButton*, gpointer data) {
    auto* self = static_cast<PlanView*>(data);
    auto* window = gtk_widget_get_ancestor(self->root_, GTK_TYPE_WINDOW);

    auto* dialog = gtk_file_dialog_new();
    gtk_file_dialog_set_title(dialog, "Open Plan File");

    auto* filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "Plan files (*.plan)");
    gtk_file_filter_add_pattern(filter, "*.plan");
    auto* filters = g_list_store_new(GTK_TYPE_FILE_FILTER);
    g_list_store_append(filters, filter);
    g_object_unref(filter);
    gtk_file_dialog_set_filters(dialog, G_LIST_MODEL(filters));
    g_object_unref(filters);

    gtk_file_dialog_open(dialog, GTK_WINDOW(window), nullptr, on_open_plan_response, self);
}

void PlanView::on_open_plan_response(GObject* source, GAsyncResult* res, gpointer data) {
    auto* self = static_cast<PlanView*>(data);
    GError* error = nullptr;
    auto* file = gtk_file_dialog_open_finish(GTK_FILE_DIALOG(source), res, &error);
    if (!file) {
        if (error) g_error_free(error);
        return;
    }

    auto* path = g_file_get_path(file);
    g_object_unref(file);

    try {
        self->project_.load_plan(path);
        auto* plan = self->current_plan();
        if (plan)
            gtk_label_set_markup(GTK_LABEL(self->plan_label_), (std::string("<b>Plan:</b> ") + plan->filepath.filename().string()).c_str());
        self->populate_task_list();
        self->update_plan_buttons();
    } catch (const std::exception& e) {
        self->project_.report_status(std::string("Error: failed to load plan: ") + e.what());
    }

    g_free(path);
}

// --- Task operations ---

void PlanView::on_task_selected(GtkListBox*, GtkListBoxRow* row, gpointer data) {
    auto* self = static_cast<PlanView*>(data);
    if (self->suppress_selection_) return;

    if (!row) {
        self->unit_editor_->clear();
        self->current_task_idx_ = -1;
        return;
    }

    int new_idx = gtk_list_box_row_get_index(row);

    if (self->unit_editor_->is_dirty() && self->current_task_idx_ >= 0 && new_idx != self->current_task_idx_) {
        // Re-select old row while dialog is showing
        self->suppress_selection_ = true;
        auto* old_row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(self->task_listbox_), self->current_task_idx_);
        if (old_row) gtk_list_box_select_row(GTK_LIST_BOX(self->task_listbox_), old_row);
        self->suppress_selection_ = false;

        auto* parent = GTK_WINDOW(gtk_widget_get_ancestor(self->root_, GTK_TYPE_WINDOW));
        auto result = show_unsaved_dialog(parent);
        if (result == UnsavedResult::Save)
            self->save_dirty();
        else
            self->revert_dirty();

        self->suppress_selection_ = true;
        auto* target_row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(self->task_listbox_), new_idx);
        if (target_row) gtk_list_box_select_row(GTK_LIST_BOX(self->task_listbox_), target_row);
        self->suppress_selection_ = false;
        self->select_task(new_idx);
        return;
    }

    self->select_task(new_idx);
}

void PlanView::on_add_task(GtkButton*, gpointer data) {
    auto* self = static_cast<PlanView*>(data);
    auto* plan = self->current_plan();
    if (!plan) return;

    // ensure units are loaded
    if (self->project_.unit_files.empty())
        self->project_.load_all_units();

    if (self->project_.unit_files.empty()) {
        self->project_.report_status("Error: no units loaded");
        return;
    }

    auto* parent = GTK_WINDOW(gtk_widget_get_ancestor(self->root_, GTK_TYPE_WINDOW));
    show_unit_picker(parent, self->project_, [self](const std::string& unit_name) {
        auto* plan = self->current_plan();
        if (plan) {
            Task t;
            t.name = unit_name;
            t.dependencies = nlohmann::json::array({nullptr});
            plan->tasks.push_back(t);
            self->plan_dirty_ = true; gtk_widget_add_css_class(self->btn_save_plan_, "suggested-action");
            self->populate_task_list();
            int last = (int)plan->tasks.size() - 1;
            auto* task_row = gtk_list_box_get_row_at_index(
                GTK_LIST_BOX(self->task_listbox_), last);
            if (task_row)
                gtk_list_box_select_row(GTK_LIST_BOX(self->task_listbox_), task_row);
        }
    });
}

void PlanView::on_delete_task(GtkButton*, gpointer data) {
    auto* self = static_cast<PlanView*>(data);
    auto* plan = self->current_plan();
    if (!plan || self->current_task_idx_ < 0) return;

    int idx = self->current_task_idx_;
    plan->tasks.erase(plan->tasks.begin() + idx);
    self->plan_dirty_ = true; gtk_widget_add_css_class(self->btn_save_plan_, "suggested-action");
    self->populate_task_list();
}

void PlanView::on_move_up(GtkButton*, gpointer data) {
    auto* self = static_cast<PlanView*>(data);
    auto* plan = self->current_plan();
    if (!plan || self->current_task_idx_ <= 0) return;

    int idx = self->current_task_idx_;
    std::swap(plan->tasks[idx], plan->tasks[idx - 1]);
    self->plan_dirty_ = true; gtk_widget_add_css_class(self->btn_save_plan_, "suggested-action");
    self->populate_task_list();

    auto* row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(self->task_listbox_), idx - 1);
    if (row)
        gtk_list_box_select_row(GTK_LIST_BOX(self->task_listbox_), row);
}

void PlanView::on_move_down(GtkButton*, gpointer data) {
    auto* self = static_cast<PlanView*>(data);
    auto* plan = self->current_plan();
    if (!plan || self->current_task_idx_ < 0 || self->current_task_idx_ >= (int)plan->tasks.size() - 1) return;

    int idx = self->current_task_idx_;
    std::swap(plan->tasks[idx], plan->tasks[idx + 1]);
    self->plan_dirty_ = true; gtk_widget_add_css_class(self->btn_save_plan_, "suggested-action");
    self->populate_task_list();

    auto* row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(self->task_listbox_), idx + 1);
    if (row)
        gtk_list_box_select_row(GTK_LIST_BOX(self->task_listbox_), row);
}

void PlanView::close_plan_impl() {
    project_.plans.clear();
    gtk_label_set_markup(GTK_LABEL(plan_label_), "<b>Plan:</b> No plan loaded");
    populate_task_list();
    update_plan_buttons();
    plan_dirty_ = false;
    gtk_widget_remove_css_class(btn_save_plan_, "suggested-action");
    project_.report_status("Plan closed");
}

void PlanView::on_close_plan(GtkButton*, gpointer data) {
    auto* self = static_cast<PlanView*>(data);
    if (self->is_dirty()) {
        auto* parent = GTK_WINDOW(gtk_widget_get_ancestor(self->root_, GTK_TYPE_WINDOW));
        auto result = show_unsaved_dialog(parent);
        if (result == UnsavedResult::Save)
            self->save_dirty();
    }
    self->close_plan_impl();
}

void PlanView::on_create_plan(GtkButton*, gpointer data) {
    auto* self = static_cast<PlanView*>(data);
    auto* window = gtk_widget_get_ancestor(self->root_, GTK_TYPE_WINDOW);

    auto* dialog = gtk_file_dialog_new();
    gtk_file_dialog_set_title(dialog, "Create Plan File");

    auto* filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "Plan files (*.plan)");
    gtk_file_filter_add_pattern(filter, "*.plan");
    auto* filters = g_list_store_new(GTK_TYPE_FILE_FILTER);
    g_list_store_append(filters, filter);
    g_object_unref(filter);
    gtk_file_dialog_set_filters(dialog, G_LIST_MODEL(filters));
    g_object_unref(filters);

    gtk_file_dialog_save(dialog, GTK_WINDOW(window), nullptr, on_create_plan_response, self);
}

void PlanView::on_create_plan_response(GObject* source, GAsyncResult* res, gpointer data) {
    auto* self = static_cast<PlanView*>(data);
    GError* error = nullptr;
    auto* file = gtk_file_dialog_save_finish(GTK_FILE_DIALOG(source), res, &error);
    if (!file) {
        if (error) g_error_free(error);
        return;
    }

    auto* path = g_file_get_path(file);
    g_object_unref(file);

    std::filesystem::path plan_path(path);
    g_free(path);

    // Ensure .plan extension
    if (plan_path.extension() != ".plan")
        plan_path += ".plan";

    // Create a new empty plan
    Plan plan;
    plan.name = plan_path.stem().string();
    plan.filepath = plan_path;

    try {
        plan.save();
        self->project_.plans.clear();
        self->project_.plans.push_back(std::move(plan));
        auto* p = self->current_plan();
        if (p)
            gtk_label_set_markup(GTK_LABEL(self->plan_label_), (std::string("<b>Plan:</b> ") + p->filepath.filename().string()).c_str());
        self->populate_task_list();
        self->update_plan_buttons();
        self->project_.report_status("Created plan: " + plan_path.filename().string());
    } catch (const std::exception& e) {
        self->project_.report_status(std::string("Error: failed to create plan: ") + e.what());
    }
}

void PlanView::update_plan_buttons() {
    bool has_plan = current_plan() != nullptr;
    gtk_widget_set_visible(btn_open_plan_, !has_plan);
    gtk_widget_set_visible(btn_create_plan_, !has_plan);
    gtk_widget_set_visible(btn_close_plan_, has_plan);
}

void PlanView::refresh() {
    // reload units if paths now resolve
    project_.load_all_units();

    auto* plan = current_plan();
    if (plan)
        gtk_label_set_markup(GTK_LABEL(plan_label_), (std::string("<b>Plan:</b> ") + plan->filepath.filename().string()).c_str());
    else
        gtk_label_set_markup(GTK_LABEL(plan_label_), "<b>Plan:</b> No plan loaded");

    populate_task_list();
    update_plan_buttons();
    plan_dirty_ = false;
    gtk_widget_remove_css_class(btn_save_plan_, "suggested-action");
}

bool PlanView::is_dirty() const {
    return plan_dirty_ || unit_editor_->is_dirty();
}

void PlanView::save_dirty() {
    if (unit_editor_->is_dirty())
        unit_editor_->save_current();
    if (plan_dirty_) {
        try {
            project_.save_plans();
            plan_dirty_ = false;
            gtk_widget_remove_css_class(btn_save_plan_, "suggested-action");
        } catch (const std::exception& e) {
            project_.report_status(std::string("Error: ") + e.what());
        }
    }
}

void PlanView::revert_dirty() {
    if (unit_editor_->is_dirty())
        unit_editor_->revert_current();
    if (plan_dirty_ && !project_.plans.empty()) {
        auto& plan = project_.plans[0];
        try {
            auto reloaded = Plan::load(plan.filepath);
            plan = std::move(reloaded);
            plan_dirty_ = false;
            gtk_widget_remove_css_class(btn_save_plan_, "suggested-action");
        } catch (const std::exception& e) {
            project_.report_status(std::string("Error: ") + e.what());
        }
    }
}

}
