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

#include "views/units_view.h"
#include "util/unsaved_dialog.h"
#include "util/unit_properties_dialog.h"
#include <cstring>
#include <fstream>

static int sort_listbox_alpha(GtkListBoxRow* a, GtkListBoxRow* b, gpointer) {
    auto* la = GTK_LABEL(gtk_list_box_row_get_child(GTK_LIST_BOX_ROW(a)));
    auto* lb = GTK_LABEL(gtk_list_box_row_get_child(GTK_LIST_BOX_ROW(b)));
    return std::strcmp(gtk_label_get_text(la), gtk_label_get_text(lb));
}

namespace grex {

UnitsView::UnitsView(Project& project, GrexConfig& grex_config)
    : project_(project), grex_config_(grex_config) {
    root_ = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_paned_set_position(GTK_PANED(root_), 300);

    // === Left panel: unit files ===
    auto* left = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_size_request(left, 200, -1);
    gtk_widget_set_margin_start(left, 8);
    gtk_widget_set_margin_end(left, 4);
    gtk_widget_set_margin_top(left, 8);
    gtk_widget_set_margin_bottom(left, 8);

    auto* file_header = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(file_header), "<b>Unit Files</b>");
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

    // File lifecycle buttons — grouped
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

    // === Right panel: units in selected file ===
    auto* right = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_margin_start(right, 4);
    gtk_widget_set_margin_end(right, 8);
    gtk_widget_set_margin_top(right, 8);
    gtk_widget_set_margin_bottom(right, 8);

    file_label_ = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(file_label_), "<b>No file selected</b>");
    gtk_label_set_xalign(GTK_LABEL(file_label_), 0.0f);
    gtk_box_append(GTK_BOX(right), file_label_);

    auto* unit_scroll = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(unit_scroll), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_vexpand(unit_scroll, TRUE);
    gtk_widget_add_css_class(unit_scroll, "frame");
    unit_listbox_ = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(unit_listbox_), GTK_SELECTION_SINGLE);
    gtk_list_box_set_activate_on_single_click(GTK_LIST_BOX(unit_listbox_), FALSE);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(unit_scroll), unit_listbox_);
    gtk_box_append(GTK_BOX(right), unit_scroll);

    // Unit action buttons — grouped by function
    auto* unit_btn_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);

    auto* unit_edit_group = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_add_css_class(unit_edit_group, "linked");
    auto* btn_new_unit = gtk_button_new_with_label("New");
    auto* btn_del_unit = gtk_button_new_with_label("Delete");
    gtk_box_append(GTK_BOX(unit_edit_group), btn_new_unit);
    gtk_box_append(GTK_BOX(unit_edit_group), btn_del_unit);
    gtk_box_append(GTK_BOX(unit_btn_box), unit_edit_group);

    auto* btn_edit_unit = gtk_button_new_with_label("Edit Unit...");
    gtk_box_append(GTK_BOX(unit_btn_box), btn_edit_unit);

    auto* move_group = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_add_css_class(move_group, "linked");
    auto* btn_move_up = gtk_button_new_with_label("Up");
    auto* btn_move_down = gtk_button_new_with_label("Down");
    gtk_box_append(GTK_BOX(move_group), btn_move_up);
    gtk_box_append(GTK_BOX(move_group), btn_move_down);
    gtk_box_append(GTK_BOX(unit_btn_box), move_group);

    gtk_box_append(GTK_BOX(right), unit_btn_box);

    gtk_paned_set_end_child(GTK_PANED(root_), right);
    gtk_paned_set_shrink_end_child(GTK_PANED(root_), FALSE);

    // Signals
    g_signal_connect(file_listbox_, "row-selected", G_CALLBACK(on_file_selected), this);
    g_signal_connect(btn_new_file, "clicked", G_CALLBACK(on_new_file), this);
    g_signal_connect(btn_del_file, "clicked", G_CALLBACK(on_delete_file), this);
    g_signal_connect(btn_new_unit, "clicked", G_CALLBACK(on_new_unit), this);
    g_signal_connect(btn_del_unit, "clicked", G_CALLBACK(on_delete_unit), this);
    g_signal_connect(btn_edit_unit, "clicked", G_CALLBACK(on_edit_unit), this);
    g_signal_connect(btn_move_up, "clicked", G_CALLBACK(on_move_up), this);
    g_signal_connect(btn_move_down, "clicked", G_CALLBACK(on_move_down), this);
    g_signal_connect(unit_listbox_, "row-activated", G_CALLBACK(on_unit_activated), this);

    g_signal_connect(btn_save_, "clicked", G_CALLBACK(+[](GtkButton*, gpointer d) {
        auto* self = static_cast<UnitsView*>(d);
        if (self->current_file_idx_ < 0 || self->current_file_idx_ >= (int)self->project_.unit_files.size()) {
            self->project_.report_status("Error: no unit file selected");
            return;
        }
        auto& uf = self->project_.unit_files[self->current_file_idx_];
        // Check for cross-file duplicates before saving
        for (auto& u : uf.units) {
            if (self->project_.is_unit_name_taken(u.name, &u)) {
                self->project_.report_status("Error: unit '" + u.name + "' conflicts with a unit in another file");
                return;
            }
        }
        try {
            uf.save();
            self->file_dirty_ = false;
            gtk_widget_remove_css_class(self->btn_save_, "suggested-action");
            self->project_.report_status("Saved unit file: " + uf.filepath.filename().string());
        } catch (const std::exception& e) {
            self->project_.report_status(std::string("Error: ") + e.what());
        }
    }), this);
}

void UnitsView::populate_file_list() {
    GtkWidget* child;
    while ((child = gtk_widget_get_first_child(file_listbox_)) != nullptr)
        gtk_list_box_remove(GTK_LIST_BOX(file_listbox_), child);

    current_file_idx_ = -1;

    for (auto& uf : project_.unit_files) {
        auto* row = gtk_list_box_row_new();
        auto text = std::string("\u25C6 ") + uf.filepath.filename().string();
        auto* label = gtk_label_new(text.c_str());
        gtk_label_set_xalign(GTK_LABEL(label), 0.0f);
        gtk_widget_set_margin_start(label, 8);
        gtk_widget_set_margin_end(label, 8);
        gtk_widget_set_margin_top(label, 4);
        gtk_widget_set_margin_bottom(label, 4);
        gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), label);
        gtk_list_box_append(GTK_LIST_BOX(file_listbox_), row);
    }

    populate_unit_list();
}

void UnitsView::populate_unit_list() {
    GtkWidget* child;
    while ((child = gtk_widget_get_first_child(unit_listbox_)) != nullptr)
        gtk_list_box_remove(GTK_LIST_BOX(unit_listbox_), child);

    if (current_file_idx_ < 0 || current_file_idx_ >= (int)project_.unit_files.size()) {
        gtk_label_set_markup(GTK_LABEL(file_label_), "<b>No file selected</b>");
        return;
    }

    auto& uf = project_.unit_files[current_file_idx_];
    gtk_label_set_markup(GTK_LABEL(file_label_),
        (std::string("<b>") + uf.filepath.filename().string() + "</b>").c_str());

    for (auto& u : uf.units) {
        auto* row = gtk_list_box_row_new();
        auto text = std::string("\u2022 ") + u.name;
        auto* label = gtk_label_new(text.c_str());
        gtk_label_set_xalign(GTK_LABEL(label), 0.0f);
        gtk_widget_set_margin_start(label, 8);
        gtk_widget_set_margin_end(label, 8);
        gtk_widget_set_margin_top(label, 4);
        gtk_widget_set_margin_bottom(label, 4);
        gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), label);
        gtk_list_box_append(GTK_LIST_BOX(unit_listbox_), row);
    }
}

void UnitsView::refresh() {
    project_.load_all_units();
    populate_file_list();
}

void UnitsView::save_current_file() {
    if (current_file_idx_ < 0 || current_file_idx_ >= (int)project_.unit_files.size())
        return;
    auto& uf = project_.unit_files[current_file_idx_];
    try {
        uf.save();
        file_dirty_ = false;
        gtk_widget_remove_css_class(btn_save_, "suggested-action");
        project_.report_status("Saved unit file: " + uf.filepath.filename().string());
    } catch (const std::exception& e) {
        project_.report_status(std::string("Error: ") + e.what());
    }
}

void UnitsView::on_file_selected(GtkListBox*, GtkListBoxRow* row, gpointer data) {
    auto* self = static_cast<UnitsView*>(data);
    if (self->suppress_selection_) return;

    if (!row) {
        self->current_file_idx_ = -1;
        self->populate_unit_list();
        return;
    }

    int new_idx = gtk_list_box_row_get_index(row);

    if (self->file_dirty_ && self->current_file_idx_ >= 0 && new_idx != self->current_file_idx_) {
        int old_idx = self->current_file_idx_;
        self->suppress_selection_ = true;
        auto* old_row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(self->file_listbox_), old_idx);
        if (old_row) gtk_list_box_select_row(GTK_LIST_BOX(self->file_listbox_), old_row);
        self->suppress_selection_ = false;

        auto* parent = GTK_WINDOW(gtk_widget_get_ancestor(self->root_, GTK_TYPE_WINDOW));
        auto result = show_unsaved_dialog(parent);
        if (result == UnsavedResult::Cancel)
            return;
        if (result == UnsavedResult::Save) {
            auto& uf = self->project_.unit_files[self->current_file_idx_];
            try { uf.save(); } catch (const std::exception& e) {
                self->project_.report_status(std::string("Error: ") + e.what());
            }
        } else {
            auto idx = self->current_file_idx_;
            if (idx >= 0 && idx < (int)self->project_.unit_files.size()) {
                auto& uf = self->project_.unit_files[idx];
                try { uf = UnitFile::load(uf.filepath); } catch (const std::exception& e) {
                    self->project_.report_status(std::string("Error: ") + e.what());
                }
            }
        }
        self->file_dirty_ = false; gtk_widget_remove_css_class(self->btn_save_, "suggested-action");

        self->suppress_selection_ = true;
        auto* target_row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(self->file_listbox_), new_idx);
        if (target_row) gtk_list_box_select_row(GTK_LIST_BOX(self->file_listbox_), target_row);
        self->suppress_selection_ = false;
        self->current_file_idx_ = new_idx;
        self->populate_unit_list();
        return;
    }

    self->current_file_idx_ = new_idx;
    self->file_dirty_ = false; gtk_widget_remove_css_class(self->btn_save_, "suggested-action");
    self->populate_unit_list();
}

void UnitsView::on_new_file(GtkButton*, gpointer data) {
    auto* self = static_cast<UnitsView*>(data);
    auto* window = gtk_widget_get_ancestor(self->root_, GTK_TYPE_WINDOW);

    auto* dialog = gtk_file_dialog_new();
    gtk_file_dialog_set_title(dialog, "Create Unit File");
    gtk_file_dialog_set_accept_label(dialog, "Create");

    auto* filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "Unit files (*.units)");
    gtk_file_filter_add_pattern(filter, "*.units");
    auto* filters = g_list_store_new(GTK_TYPE_FILE_FILTER);
    g_list_store_append(filters, filter);
    g_object_unref(filter);
    gtk_file_dialog_set_filters(dialog, G_LIST_MODEL(filters));
    g_object_unref(filters);

    auto units_dir = self->project_.resolved_units_dir();
    if (!units_dir.empty()) {
        auto* initial = g_file_new_for_path(units_dir.c_str());
        gtk_file_dialog_set_initial_folder(dialog, initial);
        g_object_unref(initial);
    }

    gtk_file_dialog_save(dialog, GTK_WINDOW(window), nullptr, on_new_file_response, self);
}

void UnitsView::on_new_file_response(GObject* source, GAsyncResult* res, gpointer data) {
    auto* self = static_cast<UnitsView*>(data);
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

    if (fp.extension() != ".units")
        fp += ".units";

    // Create empty unit file
    UnitFile uf;
    uf.name = fp.stem().string();
    uf.filepath = fp;

    try {
        uf.save();
        self->project_.unit_files.push_back(std::move(uf));
        self->populate_file_list();

        // Select the new file
        int last = (int)self->project_.unit_files.size() - 1;
        auto* row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(self->file_listbox_), last);
        if (row)
            gtk_list_box_select_row(GTK_LIST_BOX(self->file_listbox_), row);

        self->project_.report_status("Created unit file: " + fp.filename().string());
    } catch (const std::exception& e) {
        self->project_.report_status(std::string("Error: ") + e.what());
    }
}

void UnitsView::on_delete_file(GtkButton*, gpointer data) {
    auto* self = static_cast<UnitsView*>(data);
    if (self->current_file_idx_ < 0 || self->current_file_idx_ >= (int)self->project_.unit_files.size())
        return;

    auto& uf = self->project_.unit_files[self->current_file_idx_];
    auto name = uf.filepath.filename().string();

    self->project_.unit_files.erase(self->project_.unit_files.begin() + self->current_file_idx_);
    self->current_file_idx_ = -1;
    self->populate_file_list();
    self->project_.report_status("Removed unit file from project: " + name + " (file not deleted from disk)");
}

void UnitsView::on_new_unit(GtkButton*, gpointer data) {
    auto* self = static_cast<UnitsView*>(data);
    if (self->current_file_idx_ < 0 || self->current_file_idx_ >= (int)self->project_.unit_files.size()) {
        self->project_.report_status("Error: select a unit file first");
        return;
    }

    auto* parent = GTK_WINDOW(gtk_widget_get_ancestor(self->root_, GTK_TYPE_WINDOW));
    auto* win = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(win), "New Unit");
    gtk_window_set_transient_for(GTK_WINDOW(win), parent);
    gtk_window_set_modal(GTK_WINDOW(win), TRUE);
    gtk_window_set_default_size(GTK_WINDOW(win), 350, -1);

    auto* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_margin_start(box, 16);
    gtk_widget_set_margin_end(box, 16);
    gtk_widget_set_margin_top(box, 16);
    gtk_widget_set_margin_bottom(box, 16);

    auto* header = gtk_label_new("Enter a name for the new unit...");
    gtk_label_set_xalign(GTK_LABEL(header), 0.0f);
    gtk_box_append(GTK_BOX(box), header);

    auto* entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "unit_name");
    gtk_widget_set_hexpand(entry, TRUE);
    gtk_box_append(GTK_BOX(box), entry);

    auto* btn_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_widget_set_halign(btn_row, GTK_ALIGN_END);
    auto* btn_cancel = gtk_button_new_with_label("Cancel");
    auto* btn_create = gtk_button_new_with_label("Create");
    gtk_widget_add_css_class(btn_create, "suggested-action");
    gtk_box_append(GTK_BOX(btn_row), btn_cancel);
    gtk_box_append(GTK_BOX(btn_row), btn_create);
    gtk_box_append(GTK_BOX(box), btn_row);

    gtk_window_set_child(GTK_WINDOW(win), box);
    gtk_window_set_default_widget(GTK_WINDOW(win), btn_create);
    gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE);

    struct NewUnitData {
        UnitsView* view;
        GtkWidget* win;
        GtkWidget* entry;
    };
    auto* nd = new NewUnitData{self, win, entry};

    g_signal_connect(btn_cancel, "clicked", G_CALLBACK(+[](GtkButton*, gpointer d) {
        auto* nd = static_cast<NewUnitData*>(d);
        gtk_window_close(GTK_WINDOW(nd->win));
        delete nd;
    }), nd);

    auto on_create = +[](GtkButton*, gpointer d) {
        auto* nd = static_cast<NewUnitData*>(d);
        auto name = std::string(gtk_editable_get_text(GTK_EDITABLE(nd->entry)));
        if (name.empty()) {
            nd->view->project_.report_status("Error: unit name cannot be empty");
            return;
        }

        // Check for duplicate across all unit files
        if (nd->view->project_.is_unit_name_taken(name)) {
            nd->view->project_.report_status("Error: unit '" + name + "' already exists");
            return;
        }

        auto& uf = nd->view->project_.unit_files[nd->view->current_file_idx_];

        Unit u;
        u.name = name;
        uf.units.push_back(u);
        nd->view->populate_unit_list();
        nd->view->file_dirty_ = true; gtk_widget_add_css_class(nd->view->btn_save_, "suggested-action");
        nd->view->project_.report_status("Created unit: " + name);
        gtk_window_close(GTK_WINDOW(nd->win));
        delete nd;
    };
    g_signal_connect(btn_create, "clicked", G_CALLBACK(on_create), nd);

    gtk_window_present(GTK_WINDOW(win));
}

void UnitsView::on_delete_unit(GtkButton*, gpointer data) {
    auto* self = static_cast<UnitsView*>(data);
    if (self->current_file_idx_ < 0 || self->current_file_idx_ >= (int)self->project_.unit_files.size())
        return;

    auto* row = gtk_list_box_get_selected_row(GTK_LIST_BOX(self->unit_listbox_));
    if (!row) return;

    int idx = gtk_list_box_row_get_index(row);
    auto& uf = self->project_.unit_files[self->current_file_idx_];
    if (idx < 0 || idx >= (int)uf.units.size()) return;

    auto name = uf.units[idx].name;
    uf.units.erase(uf.units.begin() + idx);
    self->populate_unit_list();
    self->file_dirty_ = true; gtk_widget_add_css_class(self->btn_save_, "suggested-action");
    self->project_.report_status("Deleted unit: " + name);
}

void UnitsView::on_unit_activated(GtkListBox*, GtkListBoxRow* row, gpointer data) {
    auto* self = static_cast<UnitsView*>(data);
    if (self->current_file_idx_ < 0 || self->current_file_idx_ >= (int)self->project_.unit_files.size())
        return;

    auto& uf = self->project_.unit_files[self->current_file_idx_];

    // Get unit name from the row's label (strip bullet prefix)
    auto* label = gtk_list_box_row_get_child(GTK_LIST_BOX_ROW(row));
    auto label_text = std::string(gtk_label_get_text(GTK_LABEL(label)));
    if (label_text.rfind("\u2022 ", 0) == 0)
        label_text = label_text.substr(strlen("\u2022 "));

    // Find unit by name
    Unit* unit = nullptr;
    for (auto& u : uf.units) {
        if (u.name == label_text) { unit = &u; break; }
    }
    if (!unit) return;

    auto* parent = GTK_WINDOW(gtk_widget_get_ancestor(self->root_, GTK_TYPE_WINDOW));
    auto result = show_unit_properties_dialog(parent, unit,
        self->project_, self->grex_config_, self->project_.all_shells());

    if (result == UnitDialogResult::Save) {
        self->file_dirty_ = true;
        gtk_widget_add_css_class(self->btn_save_, "suggested-action");
        self->populate_unit_list();
    }
}

void UnitsView::on_move_up(GtkButton*, gpointer data) {
    auto* self = static_cast<UnitsView*>(data);
    if (self->current_file_idx_ < 0 || self->current_file_idx_ >= (int)self->project_.unit_files.size())
        return;

    auto* row = gtk_list_box_get_selected_row(GTK_LIST_BOX(self->unit_listbox_));
    if (!row) return;

    auto& uf = self->project_.unit_files[self->current_file_idx_];

    // Find unit index by name from the row label
    auto* label = gtk_list_box_row_get_child(GTK_LIST_BOX_ROW(row));
    auto label_text = std::string(gtk_label_get_text(GTK_LABEL(label)));
    if (label_text.rfind("\u2022 ", 0) == 0)
        label_text = label_text.substr(strlen("\u2022 "));

    int idx = -1;
    for (int i = 0; i < (int)uf.units.size(); i++) {
        if (uf.units[i].name == label_text) { idx = i; break; }
    }
    if (idx <= 0) return;

    std::swap(uf.units[idx], uf.units[idx - 1]);
    self->file_dirty_ = true;
    gtk_widget_add_css_class(self->btn_save_, "suggested-action");
    self->populate_unit_list();

    // Re-select the moved unit
    auto* new_row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(self->unit_listbox_), idx - 1);
    if (new_row) gtk_list_box_select_row(GTK_LIST_BOX(self->unit_listbox_), new_row);
}

void UnitsView::on_move_down(GtkButton*, gpointer data) {
    auto* self = static_cast<UnitsView*>(data);
    if (self->current_file_idx_ < 0 || self->current_file_idx_ >= (int)self->project_.unit_files.size())
        return;

    auto* row = gtk_list_box_get_selected_row(GTK_LIST_BOX(self->unit_listbox_));
    if (!row) return;

    auto& uf = self->project_.unit_files[self->current_file_idx_];

    // Find unit index by name from the row label
    auto* label = gtk_list_box_row_get_child(GTK_LIST_BOX_ROW(row));
    auto label_text = std::string(gtk_label_get_text(GTK_LABEL(label)));
    if (label_text.rfind("\u2022 ", 0) == 0)
        label_text = label_text.substr(strlen("\u2022 "));

    int idx = -1;
    for (int i = 0; i < (int)uf.units.size(); i++) {
        if (uf.units[i].name == label_text) { idx = i; break; }
    }
    if (idx < 0 || idx >= (int)uf.units.size() - 1) return;

    std::swap(uf.units[idx], uf.units[idx + 1]);
    self->file_dirty_ = true;
    gtk_widget_add_css_class(self->btn_save_, "suggested-action");
    self->populate_unit_list();

    // Re-select the moved unit
    auto* new_row = gtk_list_box_get_row_at_index(GTK_LIST_BOX(self->unit_listbox_), idx + 1);
    if (new_row) gtk_list_box_select_row(GTK_LIST_BOX(self->unit_listbox_), new_row);
}

void UnitsView::on_edit_unit(GtkButton*, gpointer data) {
    auto* self = static_cast<UnitsView*>(data);
    if (self->current_file_idx_ < 0 || self->current_file_idx_ >= (int)self->project_.unit_files.size())
        return;

    auto* row = gtk_list_box_get_selected_row(GTK_LIST_BOX(self->unit_listbox_));
    if (!row) {
        self->project_.report_status("Error: select a unit first");
        return;
    }

    // Get unit name from row label (strip bullet prefix)
    auto* label = gtk_list_box_row_get_child(GTK_LIST_BOX_ROW(row));
    auto label_text = std::string(gtk_label_get_text(GTK_LABEL(label)));
    if (label_text.rfind("\u2022 ", 0) == 0)
        label_text = label_text.substr(strlen("\u2022 "));

    // Find unit by name
    auto& uf = self->project_.unit_files[self->current_file_idx_];
    Unit* unit = nullptr;
    for (auto& u : uf.units) {
        if (u.name == label_text) { unit = &u; break; }
    }
    if (!unit) return;

    auto* parent = GTK_WINDOW(gtk_widget_get_ancestor(self->root_, GTK_TYPE_WINDOW));
    auto result = show_unit_properties_dialog(parent, unit,
        self->project_, self->grex_config_, self->project_.all_shells());

    if (result == UnitDialogResult::Save) {
        self->file_dirty_ = true;
        gtk_widget_add_css_class(self->btn_save_, "suggested-action");
        self->populate_unit_list();
    }
}

}
