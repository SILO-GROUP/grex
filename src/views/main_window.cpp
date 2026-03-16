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

struct TabClickData {
    MainWindow* self;
    int index;
};

MainWindow::MainWindow(GtkApplication* app, Project& project, GrexConfig& grex_config)
    : project_(project), grex_config_(grex_config) {
    window_ = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window_), "GREX: Rex Config");
    gtk_window_set_default_size(GTK_WINDOW(window_), 1200, 800);

    auto* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    // Header bar
    auto* header = gtk_header_bar_new();
    gtk_window_set_titlebar(GTK_WINDOW(window_), header);

    // Grex Config button in header bar
    auto* grex_btn = gtk_button_new_with_label("Grex Config");
    gtk_widget_remove_css_class(grex_btn, "flat");
    gtk_header_bar_pack_end(GTK_HEADER_BAR(header), grex_btn);
    g_signal_connect(grex_btn, "clicked", G_CALLBACK(on_grex_config_clicked), this);

    // Stack (no GtkStackSwitcher — we use our own buttons)
    stack_ = gtk_stack_new();
    gtk_stack_set_transition_type(GTK_STACK(stack_), GTK_STACK_TRANSITION_TYPE_SLIDE_LEFT_RIGHT);

    // Wire project status reporting to status bar
    project_.status_cb = on_project_status;
    project_.status_cb_data = this;

    // Create views
    config_view_ = new ConfigView(project_);
    config_view_->set_apply_callback(on_config_applied, this);
    gtk_stack_add_named(GTK_STACK(stack_), config_view_->widget(), "config");

    plan_view_ = new PlanView(project_, grex_config_);
    gtk_stack_add_named(GTK_STACK(stack_), plan_view_->widget(), "plans");

    units_view_ = new UnitsView(project_, grex_config_);
    gtk_stack_add_named(GTK_STACK(stack_), units_view_->widget(), "units");

    shells_view_ = new ShellsView(project_);
    gtk_stack_add_named(GTK_STACK(stack_), shells_view_->widget(), "shells");

    // Tab bar — plain buttons, we control switching
    auto* tab_bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_halign(tab_bar, GTK_ALIGN_CENTER);
    gtk_widget_add_css_class(tab_bar, "linked");

    const char* tab_labels[] = {"Rex Config", "Units", "Plans", "Shells"};
    GtkWidget* tab_pages[] = {
        config_view_->widget(), units_view_->widget(),
        plan_view_->widget(), shells_view_->widget()
    };

    for (int i = 0; i < 4; i++) {
        auto* btn = gtk_toggle_button_new_with_label(tab_labels[i]);
        gtk_box_append(GTK_BOX(tab_bar), btn);
        tabs_.push_back({tab_labels[i], tab_pages[i], btn});

        auto* data = new TabClickData{this, i};
        g_signal_connect(btn, "clicked", G_CALLBACK(+[](GtkToggleButton* btn, gpointer d) {
            auto* td = static_cast<TabClickData*>(d);
            auto* self = td->self;
            int idx = td->index;

            // If clicking the already-active tab, keep it toggled and do nothing
            if (idx == self->current_tab_) {
                gtk_toggle_button_set_active(btn, TRUE);
                return;
            }

            // Check dirty state before allowing switch
            if (!self->check_dirty_and_resolve()) {
                // Cancelled — re-toggle current tab button, untoggle this one
                self->update_tab_buttons();
                return;
            }

            self->switch_to_tab(idx);
        }), data);
    }

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tabs_[0].button), TRUE);

    gtk_box_append(GTK_BOX(vbox), tab_bar);
    gtk_widget_set_vexpand(stack_, TRUE);
    gtk_box_append(GTK_BOX(vbox), stack_);

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

void MainWindow::update_tab_buttons() {
    for (int i = 0; i < (int)tabs_.size(); i++)
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tabs_[i].button), i == current_tab_);
}

bool MainWindow::check_dirty_and_resolve() {
    auto* cur = tabs_[current_tab_].page;

    bool dirty = false;
    if (cur == config_view_->widget() && config_view_->is_dirty())
        dirty = true;
    else if (cur == plan_view_->widget() && plan_view_->is_dirty())
        dirty = true;
    else if (cur == units_view_->widget() && units_view_->is_dirty())
        dirty = true;

    if (!dirty)
        return true;

    auto result = show_unsaved_dialog(GTK_WINDOW(window_));
    if (result == UnsavedResult::Cancel)
        return false;

    if (result == UnsavedResult::Save) {
        if (cur == config_view_->widget())
            config_view_->apply_config();
        else if (cur == plan_view_->widget())
            plan_view_->save_dirty();
        else if (cur == units_view_->widget())
            units_view_->save_current_file();
    } else {
        if (cur == config_view_->widget())
            config_view_->refresh();
        else if (cur == plan_view_->widget())
            plan_view_->revert_dirty();
        else if (cur == units_view_->widget())
            units_view_->refresh();
    }
    return true;
}

void MainWindow::switch_to_tab(int idx) {
    current_tab_ = idx;
    gtk_stack_set_visible_child(GTK_STACK(stack_), tabs_[idx].page);
    update_tab_buttons();
    refresh_visible_page();
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

    gtk_widget_set_sensitive(plan_view_->widget(), has_config);
    gtk_widget_set_sensitive(units_view_->widget(), has_config);
    gtk_widget_set_sensitive(shells_view_->widget(), has_config);

    // Disable tab buttons for non-config tabs when no config loaded
    for (int i = 1; i < (int)tabs_.size(); i++)
        gtk_widget_set_sensitive(tabs_[i].button, has_config);

    // If config was just closed and we're on a non-config tab, switch to config
    if (!has_config && current_tab_ != 0)
        switch_to_tab(0);
}

void MainWindow::refresh_visible_page() {
    auto* visible = tabs_[current_tab_].page;

    // Update window title
    auto title = std::string("GREX: ") + tabs_[current_tab_].name;
    gtk_window_set_title(GTK_WINDOW(window_), title.c_str());

    // Refresh the newly visible page
    if (visible == plan_view_->widget()) {
        plan_view_->reload_plan_from_disk();
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

    auto* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 16);
    gtk_widget_set_margin_start(box, 24);
    gtk_widget_set_margin_end(box, 24);
    gtk_widget_set_margin_top(box, 20);
    gtk_widget_set_margin_bottom(box, 20);

    // Header
    auto* header = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(header), "<big><b>GREX Preferences</b></big>");
    gtk_widget_set_halign(header, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(box), header);

    auto* desc = gtk_label_new("Application settings stored in ~/.config/grex/grex.ini");
    gtk_label_set_xalign(GTK_LABEL(desc), 0.5f);
    gtk_widget_add_css_class(desc, "dim-label");
    gtk_box_append(GTK_BOX(box), desc);

    gtk_box_append(GTK_BOX(box), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

    // Settings grid
    auto* grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 12);

    auto* label = gtk_label_new("File Editor");
    gtk_label_set_xalign(GTK_LABEL(label), 1.0f);
    gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 1, 1);

    auto* entry = gtk_entry_new();
    gtk_widget_set_hexpand(entry, TRUE);
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "e.g. xdg-open, vim, code");
    gtk_editable_set_text(GTK_EDITABLE(entry), self->grex_config_.file_editor.c_str());
    gtk_grid_attach(GTK_GRID(grid), entry, 1, 0, 1, 1);

    auto* hint = gtk_label_new("Command used to open files from unit target/environment fields.");
    gtk_label_set_xalign(GTK_LABEL(hint), 0.0f);
    gtk_label_set_wrap(GTK_LABEL(hint), TRUE);
    gtk_widget_add_css_class(hint, "dim-label");
    gtk_grid_attach(GTK_GRID(grid), hint, 1, 1, 1, 1);

    gtk_box_append(GTK_BOX(box), grid);

    gtk_box_append(GTK_BOX(box), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

    // Buttons
    auto* btn_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_halign(btn_row, GTK_ALIGN_END);
    auto* btn_cancel = gtk_button_new_with_label("Cancel");
    auto* btn_save = gtk_button_new_with_label("Save");
    gtk_widget_add_css_class(btn_save, "suggested-action");
    gtk_box_append(GTK_BOX(btn_row), btn_cancel);
    gtk_box_append(GTK_BOX(btn_row), btn_save);
    gtk_box_append(GTK_BOX(box), btn_row);

    gtk_window_set_child(GTK_WINDOW(win), box);
    gtk_window_set_default_widget(GTK_WINDOW(win), btn_save);
    gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE);

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
