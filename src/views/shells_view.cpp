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
#include <cstring>

static int sort_listbox_alpha(GtkListBoxRow* a, GtkListBoxRow* b, gpointer) {
    auto* la = GTK_LABEL(gtk_list_box_row_get_child(GTK_LIST_BOX_ROW(a)));
    auto* lb = GTK_LABEL(gtk_list_box_row_get_child(GTK_LIST_BOX_ROW(b)));
    return std::strcmp(gtk_label_get_text(la), gtk_label_get_text(lb));
}

namespace grex {

ShellsView::ShellsView(Project& project) : project_(project) {
    root_ = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_paned_set_position(GTK_PANED(root_), 300);

    // === Left panel: shell files ===
    auto* left = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_size_request(left, 200, -1);
    gtk_widget_set_margin_start(left, 8);
    gtk_widget_set_margin_end(left, 4);
    gtk_widget_set_margin_top(left, 8);
    gtk_widget_set_margin_bottom(left, 8);

    auto* file_header = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(file_header), "<b>Shell Files</b>");
    gtk_label_set_xalign(GTK_LABEL(file_header), 0.0f);
    gtk_box_append(GTK_BOX(left), file_header);

    auto* file_scroll = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(file_scroll), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_vexpand(file_scroll, TRUE);
    gtk_widget_add_css_class(file_scroll, "frame");
    file_listbox_ = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(file_listbox_), GTK_SELECTION_SINGLE);
    gtk_list_box_set_sort_func(GTK_LIST_BOX(file_listbox_), sort_listbox_alpha, nullptr, nullptr);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(file_scroll), file_listbox_);
    gtk_box_append(GTK_BOX(left), file_scroll);

    auto* file_btn_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);

    auto* file_edit_group = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_add_css_class(file_edit_group, "linked");
    auto* btn_new_file = gtk_button_new_with_label("New");
    auto* btn_del_file = gtk_button_new_with_label("Delete");
    gtk_box_append(GTK_BOX(file_edit_group), btn_new_file);
    gtk_box_append(GTK_BOX(file_edit_group), btn_del_file);
    gtk_box_append(GTK_BOX(file_btn_box), file_edit_group);

    btn_save_ = gtk_button_new_with_label("Save File");
    gtk_box_append(GTK_BOX(file_btn_box), btn_save_);

    gtk_box_append(GTK_BOX(left), file_btn_box);

    gtk_paned_set_start_child(GTK_PANED(root_), left);
    gtk_paned_set_shrink_start_child(GTK_PANED(root_), FALSE);

    // === Right panel: shells in selected file + editor ===
    auto* right = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_margin_start(right, 4);
    gtk_widget_set_margin_end(right, 8);
    gtk_widget_set_margin_top(right, 8);
    gtk_widget_set_margin_bottom(right, 8);

    file_label_ = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(file_label_), "<b>No file selected</b>");
    gtk_label_set_xalign(GTK_LABEL(file_label_), 0.0f);
    gtk_box_append(GTK_BOX(right), file_label_);

    auto* shell_scroll = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(shell_scroll), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_vexpand(shell_scroll, TRUE);
    gtk_widget_add_css_class(shell_scroll, "frame");
    shell_listbox_ = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(shell_listbox_), GTK_SELECTION_SINGLE);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(shell_scroll), shell_listbox_);
    gtk_box_append(GTK_BOX(right), shell_scroll);

    // Shell action buttons
    auto* shell_btn_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);

    auto* shell_edit_group = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_add_css_class(shell_edit_group, "linked");
    auto* btn_new_shell = gtk_button_new_with_label("New");
    auto* btn_del_shell = gtk_button_new_with_label("Delete");
    gtk_box_append(GTK_BOX(shell_edit_group), btn_new_shell);
    gtk_box_append(GTK_BOX(shell_edit_group), btn_del_shell);
    gtk_box_append(GTK_BOX(shell_btn_box), shell_edit_group);

    auto* move_group = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_add_css_class(move_group, "linked");
    auto* btn_move_up = gtk_button_new_with_label("Up");
    auto* btn_move_down = gtk_button_new_with_label("Down");
    gtk_box_append(GTK_BOX(move_group), btn_move_up);
    gtk_box_append(GTK_BOX(move_group), btn_move_down);
    gtk_box_append(GTK_BOX(shell_btn_box), move_group);

    gtk_box_append(GTK_BOX(right), shell_btn_box);

    // Shell properties editor
    gtk_box_append(GTK_BOX(right), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

    auto* editor_header = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(editor_header), "<b>Shell Properties</b>");
    gtk_label_set_xalign(GTK_LABEL(editor_header), 0.0f);
    gtk_box_append(GTK_BOX(right), editor_header);

    editor_grid_ = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(editor_grid_), 8);
    gtk_grid_set_column_spacing(GTK_GRID(editor_grid_), 12);

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

    gtk_box_append(GTK_BOX(right), editor_grid_);

    gtk_paned_set_end_child(GTK_PANED(root_), right);
    gtk_paned_set_shrink_end_child(GTK_PANED(root_), FALSE);

    // Signals
    g_signal_connect(file_listbox_, "row-selected", G_CALLBACK(on_file_selected), this);
    g_signal_connect(btn_new_file, "clicked", G_CALLBACK(on_new_file), this);
    g_signal_connect(btn_del_file, "clicked", G_CALLBACK(on_delete_file), this);
    g_signal_connect(btn_new_shell, "clicked", G_CALLBACK(on_new_shell), this);
    g_signal_connect(btn_del_shell, "clicked", G_CALLBACK(on_delete_shell), this);
    g_signal_connect(btn_move_up, "clicked", G_CALLBACK(on_move_up), this);
    g_signal_connect(btn_move_down, "clicked", G_CALLBACK(on_move_down), this);

    g_signal_connect(shell_listbox_, "row-selected", G_CALLBACK(+[](GtkListBox*, GtkListBoxRow* row, gpointer d) {
        auto* self = static_cast<ShellsView*>(d);
        if (!row) { self->clear_editor(); return; }
        self->load_shell(gtk_list_box_row_get_index(row));
    }), this);

    g_signal_connect(btn_save_, "clicked", G_CALLBACK(+[](GtkButton*, gpointer d) {
        auto* self = static_cast<ShellsView*>(d);
        if (self->current_file_idx_ < 0 || self->current_file_idx_ >= (int)self->project_.shell_files.size()) {
            self->project_.report_status("Error: no shell file selected");
            return;
        }
        auto& sf = self->project_.shell_files[self->current_file_idx_];
        try {
            sf.save();
            self->file_dirty_ = false;
            gtk_widget_remove_css_class(self->btn_save_, "suggested-action");
            self->project_.report_status("Saved shell file: " + sf.filepath.filename().string());
        } catch (const std::exception& e) {
            self->project_.report_status(std::string("Error: ") + e.what());
        }
    }), this);

    // Bind entry changes to model
    auto bind = [this](GtkWidget* entry) {
        g_signal_connect(entry, "changed", G_CALLBACK(+[](GtkEditable* e, gpointer d) {
            auto* self = static_cast<ShellsView*>(d);
            if (self->loading_ || self->current_file_idx_ < 0 || self->current_shell_idx_ < 0)
                return;
            auto& sf = self->project_.shell_files[self->current_file_idx_];
            if (self->current_shell_idx_ >= (int)sf.shells.size()) return;
            auto& s = sf.shells[self->current_shell_idx_];
            auto text = std::string(gtk_editable_get_text(e));
            if (GTK_WIDGET(e) == self->entry_name_) {
                s.name = text;
                auto* row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(self->shell_listbox_), self->current_shell_idx_);
                if (row) {
                    auto* lbl = gtk_list_box_row_get_child(GTK_LIST_BOX_ROW(row));
                    if (GTK_IS_LABEL(lbl)) gtk_label_set_text(GTK_LABEL(lbl), (std::string("\u25B8 ") + text).c_str());
                }
            } else if (GTK_WIDGET(e) == self->entry_path_) s.path = text;
            else if (GTK_WIDGET(e) == self->entry_exec_arg_) s.execution_arg = text;
            else if (GTK_WIDGET(e) == self->entry_source_cmd_) s.source_cmd = text;
            self->mark_file_dirty();
        }), this);
    };
    bind(entry_name_);
    bind(entry_path_);
    bind(entry_exec_arg_);
    bind(entry_source_cmd_);

    populate_file_list();
    clear_editor();
}

void ShellsView::mark_file_dirty() {
    file_dirty_ = true;
    gtk_widget_add_css_class(btn_save_, "suggested-action");
}

void ShellsView::populate_file_list() {
    GtkWidget* child;
    while ((child = gtk_widget_get_first_child(file_listbox_)) != nullptr)
        gtk_list_box_remove(GTK_LIST_BOX(file_listbox_), child);

    current_file_idx_ = -1;

    for (auto& sf : project_.shell_files) {
        auto* row = gtk_list_box_row_new();
        auto text = std::string("\u25C6 ") + sf.filepath.filename().string();
        auto* label = gtk_label_new(text.c_str());
        gtk_label_set_xalign(GTK_LABEL(label), 0.0f);
        gtk_widget_set_margin_start(label, 8);
        gtk_widget_set_margin_end(label, 8);
        gtk_widget_set_margin_top(label, 4);
        gtk_widget_set_margin_bottom(label, 4);
        gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), label);
        gtk_list_box_append(GTK_LIST_BOX(file_listbox_), row);
    }

    populate_shell_list();
}

void ShellsView::populate_shell_list() {
    GtkWidget* child;
    while ((child = gtk_widget_get_first_child(shell_listbox_)) != nullptr)
        gtk_list_box_remove(GTK_LIST_BOX(shell_listbox_), child);

    current_shell_idx_ = -1;
    clear_editor();

    if (current_file_idx_ < 0 || current_file_idx_ >= (int)project_.shell_files.size()) {
        gtk_label_set_markup(GTK_LABEL(file_label_), "<b>No file selected</b>");
        return;
    }

    auto& sf = project_.shell_files[current_file_idx_];
    gtk_label_set_markup(GTK_LABEL(file_label_),
        (std::string("<b>") + sf.filepath.filename().string() + "</b>").c_str());

    for (auto& s : sf.shells) {
        auto* row = gtk_list_box_row_new();
        auto text = std::string("\u25B8 ") + s.name;
        auto* label = gtk_label_new(text.c_str());
        gtk_label_set_xalign(GTK_LABEL(label), 0.0f);
        gtk_widget_set_margin_start(label, 8);
        gtk_widget_set_margin_end(label, 8);
        gtk_widget_set_margin_top(label, 4);
        gtk_widget_set_margin_bottom(label, 4);
        gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), label);
        gtk_list_box_append(GTK_LIST_BOX(shell_listbox_), row);
    }
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
    if (current_file_idx_ < 0 || current_file_idx_ >= (int)project_.shell_files.size()) {
        clear_editor();
        return;
    }
    auto& sf = project_.shell_files[current_file_idx_];
    if (idx < 0 || idx >= (int)sf.shells.size()) {
        clear_editor();
        return;
    }
    loading_ = true;
    current_shell_idx_ = idx;
    auto& s = sf.shells[idx];
    gtk_widget_set_sensitive(editor_grid_, TRUE);
    gtk_editable_set_text(GTK_EDITABLE(entry_name_), s.name.c_str());
    gtk_editable_set_text(GTK_EDITABLE(entry_path_), s.path.c_str());
    gtk_editable_set_text(GTK_EDITABLE(entry_exec_arg_), s.execution_arg.c_str());
    gtk_editable_set_text(GTK_EDITABLE(entry_source_cmd_), s.source_cmd.c_str());
    loading_ = false;
}

void ShellsView::refresh() {
    project_.reload_shells();
    populate_file_list();
}

void ShellsView::on_file_selected(GtkListBox*, GtkListBoxRow* row, gpointer data) {
    auto* self = static_cast<ShellsView*>(data);
    if (!row) {
        self->current_file_idx_ = -1;
        self->populate_shell_list();
        return;
    }
    self->current_file_idx_ = gtk_list_box_row_get_index(row);
    self->file_dirty_ = false;
    gtk_widget_remove_css_class(self->btn_save_, "suggested-action");
    self->populate_shell_list();
}

void ShellsView::on_new_file(GtkButton*, gpointer data) {
    auto* self = static_cast<ShellsView*>(data);
    auto* window = gtk_widget_get_ancestor(self->root_, GTK_TYPE_WINDOW);

    auto* dialog = gtk_file_dialog_new();
    gtk_file_dialog_set_title(dialog, "Create Shell File");
    gtk_file_dialog_set_accept_label(dialog, "Create");

    auto* filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "Shell files (*.shells)");
    gtk_file_filter_add_pattern(filter, "*.shells");
    auto* filters = g_list_store_new(GTK_TYPE_FILE_FILTER);
    g_list_store_append(filters, filter);
    g_object_unref(filter);
    gtk_file_dialog_set_filters(dialog, G_LIST_MODEL(filters));
    g_object_unref(filters);

    auto shells_dir = self->project_.resolved_shells_path();
    if (!shells_dir.empty()) {
        auto* initial = g_file_new_for_path(shells_dir.c_str());
        gtk_file_dialog_set_initial_folder(dialog, initial);
        g_object_unref(initial);
    }

    gtk_file_dialog_save(dialog, GTK_WINDOW(window), nullptr, on_new_file_response, self);
}

void ShellsView::on_new_file_response(GObject* source, GAsyncResult* res, gpointer data) {
    auto* self = static_cast<ShellsView*>(data);
    GError* error = nullptr;
    auto* file = gtk_file_dialog_save_finish(GTK_FILE_DIALOG(source), res, &error);
    if (!file) {
        if (error) g_error_free(error);
        return;
    }

    auto* path = g_file_get_path(file);
    g_object_unref(file);

    std::filesystem::path fp(path);
    g_free(path);

    if (fp.extension() != ".shells")
        fp += ".shells";

    ShellsFile sf;
    sf.filepath = fp;

    try {
        sf.save();
        self->project_.shell_files.push_back(std::move(sf));
        self->populate_file_list();

        int last = (int)self->project_.shell_files.size() - 1;
        auto* row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(self->file_listbox_), last);
        if (row)
            gtk_list_box_select_row(GTK_LIST_BOX(self->file_listbox_), row);

        self->project_.report_status("Created shell file: " + fp.filename().string());
    } catch (const std::exception& e) {
        self->project_.report_status(std::string("Error: ") + e.what());
    }
}

void ShellsView::on_delete_file(GtkButton*, gpointer data) {
    auto* self = static_cast<ShellsView*>(data);
    if (self->current_file_idx_ < 0 || self->current_file_idx_ >= (int)self->project_.shell_files.size())
        return;

    auto& sf = self->project_.shell_files[self->current_file_idx_];
    auto name = sf.filepath.filename().string();

    self->project_.shell_files.erase(self->project_.shell_files.begin() + self->current_file_idx_);
    self->current_file_idx_ = -1;
    self->populate_file_list();
    self->project_.report_status("Removed shell file from project: " + name + " (file not deleted from disk)");
}

void ShellsView::on_new_shell(GtkButton*, gpointer data) {
    auto* self = static_cast<ShellsView*>(data);
    if (self->current_file_idx_ < 0 || self->current_file_idx_ >= (int)self->project_.shell_files.size()) {
        self->project_.report_status("Error: select a shell file first");
        return;
    }

    auto& sf = self->project_.shell_files[self->current_file_idx_];
    ShellDef s;
    s.name = "new_shell";
    s.path = "/usr/bin/new_shell";
    s.execution_arg = "-c";
    s.source_cmd = "source";
    sf.shells.push_back(s);
    self->populate_shell_list();
    self->mark_file_dirty();

    int last = (int)sf.shells.size() - 1;
    auto* row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(self->shell_listbox_), last);
    if (row)
        gtk_list_box_select_row(GTK_LIST_BOX(self->shell_listbox_), row);
}

void ShellsView::on_delete_shell(GtkButton*, gpointer data) {
    auto* self = static_cast<ShellsView*>(data);
    if (self->current_file_idx_ < 0 || self->current_file_idx_ >= (int)self->project_.shell_files.size())
        return;

    auto* row = gtk_list_box_get_selected_row(GTK_LIST_BOX(self->shell_listbox_));
    if (!row) return;

    int idx = gtk_list_box_row_get_index(row);
    auto& sf = self->project_.shell_files[self->current_file_idx_];
    if (idx < 0 || idx >= (int)sf.shells.size()) return;

    auto name = sf.shells[idx].name;
    sf.shells.erase(sf.shells.begin() + idx);
    self->populate_shell_list();
    self->mark_file_dirty();
    self->project_.report_status("Deleted shell: " + name);
}

void ShellsView::on_move_up(GtkButton*, gpointer data) {
    auto* self = static_cast<ShellsView*>(data);
    if (self->current_file_idx_ < 0 || self->current_file_idx_ >= (int)self->project_.shell_files.size())
        return;

    auto* row = gtk_list_box_get_selected_row(GTK_LIST_BOX(self->shell_listbox_));
    if (!row) return;

    int idx = gtk_list_box_row_get_index(row);
    auto& sf = self->project_.shell_files[self->current_file_idx_];
    if (idx <= 0) return;

    std::swap(sf.shells[idx], sf.shells[idx - 1]);
    self->mark_file_dirty();
    self->populate_shell_list();

    auto* new_row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(self->shell_listbox_), idx - 1);
    if (new_row) gtk_list_box_select_row(GTK_LIST_BOX(self->shell_listbox_), new_row);
}

void ShellsView::on_move_down(GtkButton*, gpointer data) {
    auto* self = static_cast<ShellsView*>(data);
    if (self->current_file_idx_ < 0 || self->current_file_idx_ >= (int)self->project_.shell_files.size())
        return;

    auto* row = gtk_list_box_get_selected_row(GTK_LIST_BOX(self->shell_listbox_));
    if (!row) return;

    int idx = gtk_list_box_row_get_index(row);
    auto& sf = self->project_.shell_files[self->current_file_idx_];
    if (idx < 0 || idx >= (int)sf.shells.size() - 1) return;

    std::swap(sf.shells[idx], sf.shells[idx + 1]);
    self->mark_file_dirty();
    self->populate_shell_list();

    auto* new_row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(self->shell_listbox_), idx + 1);
    if (new_row) gtk_list_box_select_row(GTK_LIST_BOX(self->shell_listbox_), new_row);
}

}
