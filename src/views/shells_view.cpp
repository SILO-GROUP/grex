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

#include "views/shells_view.h"

namespace grex {

ShellsView::ShellsView(Project& project) : project_(project), shells_(project.shells) {
    root_ = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_paned_set_position(GTK_PANED(root_), 200);

    // Left panel: shell list
    auto* left = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_widget_set_size_request(left, 150, -1);

    auto* scroll = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_vexpand(scroll, TRUE);
    listbox_ = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(listbox_), GTK_SELECTION_SINGLE);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), listbox_);
    gtk_box_append(GTK_BOX(left), scroll);

    auto* btn_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_widget_set_margin_start(btn_box, 4);
    gtk_widget_set_margin_end(btn_box, 4);
    gtk_widget_set_margin_bottom(btn_box, 4);
    auto* btn_add = gtk_button_new_with_label("Add");
    auto* btn_del = gtk_button_new_with_label("Delete");
    auto* btn_save = gtk_button_new_with_label("Save Shells");
    gtk_box_append(GTK_BOX(btn_box), btn_add);
    gtk_box_append(GTK_BOX(btn_box), btn_del);
    gtk_box_append(GTK_BOX(btn_box), btn_save);
    gtk_box_append(GTK_BOX(left), btn_box);

    gtk_paned_set_start_child(GTK_PANED(root_), left);
    gtk_paned_set_shrink_start_child(GTK_PANED(root_), FALSE);

    // Right panel: editor
    auto* right_scroll = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(right_scroll), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

    editor_grid_ = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(editor_grid_), 8);
    gtk_grid_set_column_spacing(GTK_GRID(editor_grid_), 12);
    gtk_widget_set_margin_start(editor_grid_, 16);
    gtk_widget_set_margin_end(editor_grid_, 16);
    gtk_widget_set_margin_top(editor_grid_, 16);
    gtk_widget_set_margin_bottom(editor_grid_, 16);

    auto make_row = [&](int row, const char* label_text) -> GtkWidget* {
        auto* label = gtk_label_new(label_text);
        gtk_label_set_xalign(GTK_LABEL(label), 1.0f);
        gtk_grid_attach(GTK_GRID(editor_grid_), label, 0, row, 1, 1);
        auto* entry = gtk_entry_new();
        gtk_widget_set_hexpand(entry, TRUE);
        gtk_grid_attach(GTK_GRID(editor_grid_), entry, 1, row, 1, 1);
        return entry;
    };

    entry_name_ = make_row(0, "Name");
    entry_path_ = make_row(1, "Path");
    entry_exec_arg_ = make_row(2, "Execution Arg");
    entry_source_cmd_ = make_row(3, "Source Cmd");

    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(right_scroll), editor_grid_);
    gtk_paned_set_end_child(GTK_PANED(root_), right_scroll);
    gtk_paned_set_shrink_end_child(GTK_PANED(root_), FALSE);

    // Signals
    g_signal_connect(listbox_, "row-selected", G_CALLBACK(on_shell_selected), this);
    g_signal_connect(btn_add, "clicked", G_CALLBACK(on_add_shell), this);
    g_signal_connect(btn_del, "clicked", G_CALLBACK(on_delete_shell), this);

    g_signal_connect(btn_save, "clicked", G_CALLBACK(+[](GtkButton*, gpointer d) {
        auto* self = static_cast<ShellsView*>(d);
        try {
            self->project_.save_shells();
        } catch (const std::exception& e) {
            self->project_.report_status(std::string("Error: ") + e.what());
        }
    }), this);

    // Bind entry changes to model
    auto bind = [this](GtkWidget* entry, int field) {
        g_signal_connect(entry, "changed", G_CALLBACK(+[](GtkEditable* e, gpointer d) {
            auto* self = static_cast<ShellsView*>(d);
            if (self->loading_ || self->current_idx_ < 0 || self->current_idx_ >= (int)self->shells_.shells.size())
                return;
            auto& s = self->shells_.shells[self->current_idx_];
            auto text = std::string(gtk_editable_get_text(e));
            // determine which field by checking widget pointer
            if (GTK_WIDGET(e) == self->entry_name_) {
                s.name = text;
                // refresh list row
                auto* row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(self->listbox_), self->current_idx_);
                if (row) {
                    auto* lbl = gtk_list_box_row_get_child(GTK_LIST_BOX_ROW(row));
                    if (GTK_IS_LABEL(lbl)) gtk_label_set_text(GTK_LABEL(lbl), text.c_str());
                }
            } else if (GTK_WIDGET(e) == self->entry_path_) s.path = text;
            else if (GTK_WIDGET(e) == self->entry_exec_arg_) s.execution_arg = text;
            else if (GTK_WIDGET(e) == self->entry_source_cmd_) s.source_cmd = text;
        }), this);
    };
    bind(entry_name_, 0);
    bind(entry_path_, 1);
    bind(entry_exec_arg_, 2);
    bind(entry_source_cmd_, 3);

    populate_list();
    clear_editor();
}

void ShellsView::populate_list() {
    GtkWidget* child;
    while ((child = gtk_widget_get_first_child(listbox_)) != nullptr)
        gtk_list_box_remove(GTK_LIST_BOX(listbox_), child);

    for (auto& s : shells_.shells) {
        auto* row = gtk_list_box_row_new();
        auto stext = std::string("\u25B8 ") + s.name;
        auto* label = gtk_label_new(stext.c_str());
        gtk_label_set_xalign(GTK_LABEL(label), 0.0f);
        gtk_widget_set_margin_start(label, 8);
        gtk_widget_set_margin_top(label, 4);
        gtk_widget_set_margin_bottom(label, 4);
        gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), label);
        gtk_list_box_append(GTK_LIST_BOX(listbox_), row);
    }

    current_idx_ = -1;
    clear_editor();
}

void ShellsView::clear_editor() {
    loading_ = true;
    gtk_editable_set_text(GTK_EDITABLE(entry_name_), "");
    gtk_editable_set_text(GTK_EDITABLE(entry_path_), "");
    gtk_editable_set_text(GTK_EDITABLE(entry_exec_arg_), "");
    gtk_editable_set_text(GTK_EDITABLE(entry_source_cmd_), "");
    gtk_widget_set_sensitive(editor_grid_, FALSE);
    loading_ = false;
}

void ShellsView::load_shell(int idx) {
    if (idx < 0 || idx >= (int)shells_.shells.size()) {
        clear_editor();
        return;
    }
    loading_ = true;
    current_idx_ = idx;
    auto& s = shells_.shells[idx];
    gtk_widget_set_sensitive(editor_grid_, TRUE);
    gtk_editable_set_text(GTK_EDITABLE(entry_name_), s.name.c_str());
    gtk_editable_set_text(GTK_EDITABLE(entry_path_), s.path.c_str());
    gtk_editable_set_text(GTK_EDITABLE(entry_exec_arg_), s.execution_arg.c_str());
    gtk_editable_set_text(GTK_EDITABLE(entry_source_cmd_), s.source_cmd.c_str());
    loading_ = false;
}

void ShellsView::on_shell_selected(GtkListBox*, GtkListBoxRow* row, gpointer data) {
    auto* self = static_cast<ShellsView*>(data);
    if (!row) { self->clear_editor(); return; }
    self->load_shell(gtk_list_box_row_get_index(row));
}

void ShellsView::on_add_shell(GtkButton*, gpointer data) {
    auto* self = static_cast<ShellsView*>(data);
    ShellDef s;
    s.name = "new_shell";
    s.path = "/usr/bin/new_shell";
    s.execution_arg = "-c";
    s.source_cmd = "source";
    self->shells_.shells.push_back(s);
    self->populate_list();

    int last = (int)self->shells_.shells.size() - 1;
    auto* row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(self->listbox_), last);
    if (row)
        gtk_list_box_select_row(GTK_LIST_BOX(self->listbox_), row);
}

void ShellsView::on_delete_shell(GtkButton*, gpointer data) {
    auto* self = static_cast<ShellsView*>(data);
    if (self->current_idx_ < 0) return;
    self->shells_.shells.erase(self->shells_.shells.begin() + self->current_idx_);
    self->populate_list();
}

void ShellsView::refresh() {
    populate_list();
}

}
