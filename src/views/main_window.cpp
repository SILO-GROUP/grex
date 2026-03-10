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

#include "views/main_window.h"
#include "views/config_view.h"
#include "views/plan_view.h"
#include "views/units_view.h"
#include "views/shells_view.h"
#include "util/unsaved_dialog.h"

namespace grex {

MainWindow::MainWindow(GtkApplication* app, Project& project, GrexConfig& grex_config)
    : project_(project), grex_config_(grex_config) {
    window_ = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window_), "grex");
    gtk_window_set_default_size(GTK_WINDOW(window_), 1200, 800);

    auto* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    // Header bar
    auto* header = gtk_header_bar_new();
    gtk_window_set_title(GTK_WINDOW(window_), "GREX: Rex Config");
    gtk_window_set_titlebar(GTK_WINDOW(window_), header);

    // Grex Config button in header bar (opens modal dialog)
    auto* grex_btn = gtk_button_new_with_label("Grex Config");
    gtk_widget_remove_css_class(grex_btn, "flat");
    gtk_header_bar_pack_end(GTK_HEADER_BAR(header), grex_btn);
    g_signal_connect(grex_btn, "clicked", G_CALLBACK(on_grex_config_clicked), this);

    // Stack + switcher
    stack_ = gtk_stack_new();
    auto* stack = stack_;
    gtk_stack_set_transition_type(GTK_STACK(stack), GTK_STACK_TRANSITION_TYPE_SLIDE_LEFT_RIGHT);

    switcher_ = gtk_stack_switcher_new();
    gtk_stack_switcher_set_stack(GTK_STACK_SWITCHER(switcher_), GTK_STACK(stack));
    gtk_widget_set_halign(switcher_, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(vbox), switcher_);

    // Wire project status reporting to status bar
    project_.status_cb = on_project_status;
    project_.status_cb_data = this;

    // Rex Config view
    config_view_ = new ConfigView(project_);
    config_view_->set_apply_callback(on_config_applied, this);
    gtk_stack_add_titled(GTK_STACK(stack), config_view_->widget(), "config", "Rex Config");

    // Plan view
    plan_view_ = new PlanView(project_, grex_config_);
    gtk_stack_add_titled(GTK_STACK(stack), plan_view_->widget(), "plans", "Plans");

    // Units view
    units_view_ = new UnitsView(project_, grex_config_);
    gtk_stack_add_titled(GTK_STACK(stack), units_view_->widget(), "units", "Units");

    // Shells view
    shells_view_ = new ShellsView(project_);
    gtk_stack_add_titled(GTK_STACK(stack), shells_view_->widget(), "shells", "Shells");

    gtk_widget_set_vexpand(stack, TRUE);
    gtk_box_append(GTK_BOX(vbox), stack);

    g_signal_connect(stack, "notify::visible-child", G_CALLBACK(on_stack_page_changed), this);

    // Status bar
    gtk_box_append(GTK_BOX(vbox), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));
    auto* status_bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_size_request(status_bar, -1, 28);
    status_label_ = gtk_label_new("Ready");
    gtk_label_set_xalign(GTK_LABEL(status_label_), 0.0f);
    gtk_label_set_ellipsize(GTK_LABEL(status_label_), PANGO_ELLIPSIZE_END);
    gtk_label_set_selectable(GTK_LABEL(status_label_), TRUE);
    gtk_widget_set_hexpand(status_label_, TRUE);
    gtk_widget_set_margin_start(status_label_, 8);
    gtk_widget_set_margin_end(status_label_, 8);
    gtk_box_append(GTK_BOX(status_bar), status_label_);
    gtk_box_append(GTK_BOX(vbox), status_bar);

    prev_page_ = config_view_->widget();

    gtk_window_set_child(GTK_WINDOW(window_), vbox);

    update_tab_sensitivity();
}

MainWindow::~MainWindow() {
    delete config_view_;
    delete plan_view_;
    delete units_view_;
    delete shells_view_;
}

void MainWindow::set_status(const std::string& msg) {
    gtk_label_set_text(GTK_LABEL(status_label_), msg.c_str());
}

void MainWindow::on_config_applied(void* data) {
    auto* self = static_cast<MainWindow*>(data);
    self->update_tab_sensitivity();
    self->plan_view_->refresh();
    self->units_view_->refresh();
    self->shells_view_->refresh();
}

void MainWindow::update_tab_sensitivity() {
    bool has_config = !project_.config_path.empty();

    // Enable/disable the page content
    gtk_widget_set_sensitive(plan_view_->widget(), has_config);
    gtk_widget_set_sensitive(units_view_->widget(), has_config);
    gtk_widget_set_sensitive(shells_view_->widget(), has_config);

    // Grey out the switcher buttons for disabled tabs
    // GtkStackSwitcher children are toggle buttons in page order
    auto* child = gtk_widget_get_first_child(switcher_);
    int i = 0;
    while (child) {
        // child 0 = Rex Config (always enabled), 1-3 = Plans/Units/Shells
        if (i > 0)
            gtk_widget_set_sensitive(child, has_config);
        child = gtk_widget_get_next_sibling(child);
        i++;
    }

    // If config was just closed and we're on a non-config tab, switch to config
    if (!has_config) {
        auto* visible = gtk_stack_get_visible_child(GTK_STACK(stack_));
        if (visible != config_view_->widget())
            gtk_stack_set_visible_child(GTK_STACK(stack_), config_view_->widget());
    }
}

void MainWindow::on_stack_page_changed(GObject* stack, GParamSpec*, gpointer data) {
    auto* self = static_cast<MainWindow*>(data);
    auto* visible = gtk_stack_get_visible_child(GTK_STACK(stack));

    // Check if previous page has unsaved changes
    auto* prev = self->prev_page_;
    self->prev_page_ = visible;

    bool prev_dirty = false;
    if (prev == self->config_view_->widget() && self->config_view_->is_dirty())
        prev_dirty = true;
    else if (prev == self->plan_view_->widget() && self->plan_view_->is_dirty())
        prev_dirty = true;
    else if (prev == self->units_view_->widget() && self->units_view_->is_dirty())
        prev_dirty = true;

    if (prev_dirty) {
        auto result = show_unsaved_dialog(GTK_WINDOW(self->window_));
        if (result == UnsavedResult::Save) {
            if (self->config_view_->is_dirty())
                self->config_view_->apply_config();
            if (self->plan_view_->is_dirty())
                self->plan_view_->save_dirty();
            if (self->units_view_->is_dirty())
                self->units_view_->save_current_file();
        } else {
            if (self->config_view_->is_dirty())
                self->config_view_->refresh();
            if (self->plan_view_->is_dirty())
                self->plan_view_->revert_dirty();
            if (self->units_view_->is_dirty())
                self->units_view_->refresh();
        }
    }

    // No dirty state — refresh immediately
    self->refresh_visible_page();
}

void MainWindow::refresh_visible_page() {
    auto* visible = gtk_stack_get_visible_child(GTK_STACK(stack_));

    // Update window title for current tab
    if (visible == config_view_->widget())
        gtk_window_set_title(GTK_WINDOW(window_), "GREX: Rex Config");
    else if (visible == plan_view_->widget())
        gtk_window_set_title(GTK_WINDOW(window_), "GREX: Plans");
    else if (visible == units_view_->widget())
        gtk_window_set_title(GTK_WINDOW(window_), "GREX: Units");
    else if (visible == shells_view_->widget())
        gtk_window_set_title(GTK_WINDOW(window_), "GREX: Shells");

    // Refresh the newly visible page
    if (visible == plan_view_->widget()) {
        project_.load_all_units();
        if (!project_.plans.empty()) {
            auto& plan = project_.plans[0];
            try {
                plan = Plan::load(plan.filepath);
            } catch (const std::exception& e) {
                project_.report_status(std::string("Error reloading plan: ") + e.what());
            }
        }
        plan_view_->refresh();
    } else if (visible == units_view_->widget()) {
        units_view_->refresh();
    } else if (visible == shells_view_->widget()) {
        project_.reload_shells();
        shells_view_->refresh();
    }
}

void MainWindow::on_project_status(const std::string& msg, void* data) {
    auto* self = static_cast<MainWindow*>(data);
    self->set_status(msg);
}

struct GCDialogData {
    MainWindow* main;
    GtkWidget* win;
    GtkWidget* entry;
};

void MainWindow::on_grex_config_clicked(GtkButton*, gpointer data) {
    auto* self = static_cast<MainWindow*>(data);
    auto* win = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(win), "Grex Config");
    gtk_window_set_transient_for(GTK_WINDOW(win), GTK_WINDOW(self->window_));
    gtk_window_set_modal(GTK_WINDOW(win), TRUE);
    gtk_window_set_default_size(GTK_WINDOW(win), 400, -1);

    auto* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_margin_start(box, 16);
    gtk_widget_set_margin_end(box, 16);
    gtk_widget_set_margin_top(box, 16);
    gtk_widget_set_margin_bottom(box, 16);

    auto* grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 8);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 12);

    auto* label = gtk_label_new("File Editor");
    gtk_label_set_xalign(GTK_LABEL(label), 1.0f);
    gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 1, 1);

    auto* entry = gtk_entry_new();
    gtk_widget_set_hexpand(entry, TRUE);
    gtk_editable_set_text(GTK_EDITABLE(entry), self->grex_config_.file_editor.c_str());
    gtk_grid_attach(GTK_GRID(grid), entry, 1, 0, 1, 1);

    gtk_box_append(GTK_BOX(box), grid);

    auto* btn_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_widget_set_halign(btn_row, GTK_ALIGN_END);
    auto* btn_cancel = gtk_button_new_with_label("Cancel");
    auto* btn_save = gtk_button_new_with_label("Save");
    gtk_box_append(GTK_BOX(btn_row), btn_cancel);
    gtk_box_append(GTK_BOX(btn_row), btn_save);
    gtk_box_append(GTK_BOX(box), btn_row);

    gtk_window_set_child(GTK_WINDOW(win), box);

    auto* dd = new GCDialogData{self, win, entry};

    g_signal_connect(btn_cancel, "clicked", G_CALLBACK(+[](GtkButton*, gpointer d) {
        auto* dd = static_cast<GCDialogData*>(d);
        gtk_window_close(GTK_WINDOW(dd->win));
        delete dd;
    }), dd);

    auto on_save = +[](GtkButton*, gpointer d) {
        auto* dd = static_cast<GCDialogData*>(d);
        dd->main->grex_config_.file_editor = gtk_editable_get_text(GTK_EDITABLE(dd->entry));
        try {
            dd->main->grex_config_.save();
            dd->main->set_status("Saved grex config");
        } catch (const std::exception& e) {
            dd->main->set_status(std::string("Error saving grex config: ") + e.what());
        }
        gtk_window_close(GTK_WINDOW(dd->win));
        delete dd;
    };
    g_signal_connect(btn_save, "clicked", G_CALLBACK(on_save), dd);

    gtk_window_present(GTK_WINDOW(win));
}

}
