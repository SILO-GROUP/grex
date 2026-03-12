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

#include "views/config_view.h"
#include "util/unsaved_dialog.h"
#include "util/json_helpers.h"
#include <cstdlib>
#include <filesystem>

namespace grex {

// binding structs to avoid commas in g_signal_connect macro args
struct CfgFieldBinding {
    ConfigView* view;
    std::string key;
};

struct CfgVarBinding {
    ConfigView* view;
    std::string var_name;
};

ConfigView::ConfigView(Project& project) : project_(project) {
    root_ = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(root_), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

    auto* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_margin_start(box, 16);
    gtk_widget_set_margin_end(box, 16);
    gtk_widget_set_margin_top(box, 16);
    gtk_widget_set_margin_bottom(box, 16);

    // === Config file label + Open/Close buttons ===
    config_label_ = gtk_label_new(nullptr);
    gtk_label_set_xalign(GTK_LABEL(config_label_), 0.0f);
    gtk_box_append(GTK_BOX(box), config_label_);

    auto* btn_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_add_css_class(btn_row, "linked");
    btn_open_ = gtk_button_new_with_label("Open");
    btn_create_ = gtk_button_new_with_label("Create");
    btn_close_ = gtk_button_new_with_label("Close");
    gtk_widget_set_hexpand(btn_open_, TRUE);
    gtk_widget_set_hexpand(btn_create_, TRUE);
    gtk_widget_set_hexpand(btn_close_, TRUE);
    gtk_box_append(GTK_BOX(btn_row), btn_open_);
    gtk_box_append(GTK_BOX(btn_row), btn_create_);
    gtk_box_append(GTK_BOX(btn_row), btn_close_);
    gtk_box_append(GTK_BOX(box), btn_row);

    g_signal_connect(btn_open_, "clicked", G_CALLBACK(on_open_config), this);
    g_signal_connect(btn_create_, "clicked", G_CALLBACK(on_create_config), this);
    g_signal_connect(btn_close_, "clicked", G_CALLBACK(on_close_config), this);

    // === Config content — greyed out when no config loaded ===
    config_content_ = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);

    gtk_box_append(GTK_BOX(config_content_), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

    // === Config fields section ===
    auto* config_header = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(config_header), "<b>Configuration</b>");
    gtk_label_set_xalign(GTK_LABEL(config_header), 0.0f);
    gtk_box_append(GTK_BOX(config_content_), config_header);

    config_grid_ = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(config_grid_), 8);
    gtk_grid_set_column_spacing(GTK_GRID(config_grid_), 12);
    gtk_box_append(GTK_BOX(config_content_), config_grid_);

    build_config_fields();

    // === Resolved paths section ===
    gtk_box_append(GTK_BOX(config_content_), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

    auto* resolved_header = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(resolved_header), "<b>Resolved Paths</b>");
    gtk_label_set_xalign(GTK_LABEL(resolved_header), 0.0f);
    gtk_box_append(GTK_BOX(config_content_), resolved_header);

    resolved_grid_ = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(resolved_grid_), 4);
    gtk_grid_set_column_spacing(GTK_GRID(resolved_grid_), 12);
    gtk_box_append(GTK_BOX(config_content_), resolved_grid_);

    // === Variables section ===
    gtk_box_append(GTK_BOX(config_content_), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

    auto* vars_header = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(vars_header), "<b>Variables</b>");
    gtk_label_set_xalign(GTK_LABEL(vars_header), 0.0f);
    gtk_box_append(GTK_BOX(config_content_), vars_header);

    auto* vars_desc = gtk_label_new("Set values for variables found in config fields. Environment variables are used automatically.");
    gtk_label_set_xalign(GTK_LABEL(vars_desc), 0.0f);
    gtk_label_set_wrap(GTK_LABEL(vars_desc), TRUE);
    gtk_widget_add_css_class(vars_desc, "dim-label");
    gtk_box_append(GTK_BOX(config_content_), vars_desc);

    vars_grid_ = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(vars_grid_), 8);
    gtk_grid_set_column_spacing(GTK_GRID(vars_grid_), 12);
    gtk_box_append(GTK_BOX(config_content_), vars_grid_);

    build_variables_section();
    update_resolved_labels();

    // === Save button ===
    gtk_box_append(GTK_BOX(config_content_), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

    btn_save_ = gtk_button_new_with_label("Save Config");
    gtk_widget_set_halign(btn_save_, GTK_ALIGN_END);
    gtk_box_append(GTK_BOX(config_content_), btn_save_);

    g_signal_connect(btn_save_, "clicked", G_CALLBACK(+[](GtkButton*, gpointer d) {
        static_cast<ConfigView*>(d)->apply_config();
    }), this);

    gtk_box_append(GTK_BOX(box), config_content_);

    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(root_), box);

    update_config_buttons();
}

void ConfigView::build_config_fields() {
    // free old bindings
    for (auto* p : field_bindings_) delete static_cast<CfgFieldBinding*>(p);
    field_bindings_.clear();

    // clear grid
    GtkWidget* child;
    while ((child = gtk_widget_get_first_child(config_grid_)) != nullptr)
        gtk_grid_remove(GTK_GRID(config_grid_), child);

    // iterate all keys in the config JSON object
    int row = 0;
    for (auto& [key, val] : project_.config.data().items()) {
        auto* label = gtk_label_new(key.c_str());
        gtk_label_set_xalign(GTK_LABEL(label), 1.0f);
        gtk_grid_attach(GTK_GRID(config_grid_), label, 0, row, 1, 1);

        auto* entry = gtk_entry_new();
        if (val.is_string())
            gtk_editable_set_text(GTK_EDITABLE(entry), val.get<std::string>().c_str());
        else
            gtk_editable_set_text(GTK_EDITABLE(entry), val.dump().c_str());
        gtk_widget_set_hexpand(entry, TRUE);
        gtk_grid_attach(GTK_GRID(config_grid_), entry, 1, row, 1, 1);

        auto* binding = new CfgFieldBinding{this, key};
        field_bindings_.push_back(binding);
        g_signal_connect(entry, "changed", G_CALLBACK(+[](GtkEditable* e, gpointer d) {
            auto* b = static_cast<CfgFieldBinding*>(d);
            b->view->project_.config.set(b->key, gtk_editable_get_text(e));
            // rescan all values for variables
            b->view->project_.resolver.scan_and_populate(
                b->view->project_.config.all_string_values());
            b->view->rebuild_variables();
            b->view->update_resolved_labels();
            if (!b->view->rebuilding_) { b->view->dirty_ = true; gtk_widget_add_css_class(b->view->btn_save_, "suggested-action"); }
        }), binding);

        row++;
    }
}

void ConfigView::rebuild_variables() {
    build_variables_section();
}

void ConfigView::build_variables_section() {
    rebuilding_ = true;

    // free old bindings
    for (auto* p : var_bindings_) delete static_cast<CfgVarBinding*>(p);
    var_bindings_.clear();

    GtkWidget* child;
    while ((child = gtk_widget_get_first_child(vars_grid_)) != nullptr)
        gtk_grid_remove(GTK_GRID(vars_grid_), child);

    auto& overrides = project_.resolver.overrides();
    if (overrides.empty()) {
        auto* lbl = gtk_label_new("No variables found in config values.");
        gtk_label_set_xalign(GTK_LABEL(lbl), 0.0f);
        gtk_grid_attach(GTK_GRID(vars_grid_), lbl, 0, 0, 2, 1);
        rebuilding_ = false;
        return;
    }

    int row = 0;
    for (auto& [name, value] : overrides) {
        auto var_label = "${" + name + "}";
        auto* lbl = gtk_label_new(var_label.c_str());
        gtk_label_set_xalign(GTK_LABEL(lbl), 1.0f);
        gtk_grid_attach(GTK_GRID(vars_grid_), lbl, 0, row, 1, 1);

        auto* entry = gtk_entry_new();
        gtk_widget_set_hexpand(entry, TRUE);

        // populate from override first, then check live env
        const char* env_val = std::getenv(name.c_str());
        if (!value.empty()) {
            gtk_editable_set_text(GTK_EDITABLE(entry), value.c_str());
        } else if (env_val) {
            gtk_editable_set_text(GTK_EDITABLE(entry), env_val);
            // store the env value so resolver uses it
            project_.resolver.set(name, env_val);
        }

        if (env_val) {
            auto hint = std::string("from env: ") + env_val;
            gtk_widget_set_tooltip_text(entry, hint.c_str());
        }

        gtk_grid_attach(GTK_GRID(vars_grid_), entry, 1, row, 1, 1);

        auto* binding = new CfgVarBinding{this, name};
        var_bindings_.push_back(binding);
        g_signal_connect(entry, "changed", G_CALLBACK(+[](GtkEditable* e, gpointer d) {
            auto* b = static_cast<CfgVarBinding*>(d);
            if (b->view->rebuilding_) return;
            b->view->project_.resolver.set(b->var_name, gtk_editable_get_text(e));
            b->view->update_resolved_labels();
        }), binding);

        row++;
    }

    rebuilding_ = false;
}

void ConfigView::update_resolved_labels() {
    // clear existing rows
    GtkWidget* child;
    while ((child = gtk_widget_get_first_child(resolved_grid_)) != nullptr)
        gtk_grid_remove(GTK_GRID(resolved_grid_), child);

    int row = 0;
    auto root = project_.resolved_project_root();

    for (auto& [key, val] : project_.config.data().items()) {
        if (!val.is_string()) continue;
        auto raw = val.get<std::string>();

        bool has_vars = !VarResolver::find_variables(raw).empty();
        bool is_path_key = key.find("path") != std::string::npos ||
                           key.find("root") != std::string::npos ||
                           key.find("dir") != std::string::npos;
        if (!has_vars && !is_path_key) continue;

        std::string display;
        if (has_vars && !project_.resolver.can_resolve(raw)) {
            display = "(unresolved)";
        } else {
            auto resolved = project_.resolver.resolve(raw);
            // relative paths get combined with project root (except project_root itself)
            if (!root.empty() && !resolved.empty() && resolved[0] != '/' && key != "project_root")
                display = (root / resolved).string();
            else
                display = resolved;
        }

        auto* lbl = gtk_label_new(key.c_str());
        gtk_label_set_xalign(GTK_LABEL(lbl), 1.0f);
        gtk_grid_attach(GTK_GRID(resolved_grid_), lbl, 0, row, 1, 1);
        auto* val_label = gtk_label_new(nullptr);
        gtk_label_set_xalign(GTK_LABEL(val_label), 0.0f);
        gtk_label_set_selectable(GTK_LABEL(val_label), TRUE);
        gtk_widget_set_hexpand(val_label, TRUE);

        if (display == "(unresolved)") {
            gtk_label_set_markup(GTK_LABEL(val_label),
                "<span foreground=\"red\">(unresolved)</span>");
        } else if (std::filesystem::is_directory(display)) {
            auto markup = "<span foreground=\"green\">" + display + "</span>";
            gtk_label_set_markup(GTK_LABEL(val_label), markup.c_str());
        } else if (std::filesystem::exists(display)) {
            auto markup = "<span foreground=\"green\">" + display + "</span>";
            gtk_label_set_markup(GTK_LABEL(val_label), markup.c_str());
        } else {
            auto markup = "<span foreground=\"red\">" + display + "</span>";
            gtk_label_set_markup(GTK_LABEL(val_label), markup.c_str());
        }

        gtk_grid_attach(GTK_GRID(resolved_grid_), val_label, 1, row, 1, 1);
        row++;
    }

    if (row == 0) {
        auto* lbl = gtk_label_new("No resolvable paths in config.");
        gtk_label_set_xalign(GTK_LABEL(lbl), 0.0f);
        gtk_grid_attach(GTK_GRID(resolved_grid_), lbl, 0, 0, 2, 1);
    }
}

void ConfigView::set_apply_callback(ApplyCallback cb, void* data) {
    apply_cb_ = cb;
    apply_cb_data_ = data;
}

void ConfigView::apply_config() {
    namespace fs = std::filesystem;

    try {
        project_.config.save(project_.config_path);
        dirty_ = false;
        gtk_widget_remove_css_class(btn_save_, "suggested-action");
        project_.report_status("Config saved: " + project_.config_path.filename().string());

        // reload shells if path now resolves
        project_.reload_shells();

        // notify MainWindow to refresh other views
        if (apply_cb_)
            apply_cb_(apply_cb_data_);
    } catch (const std::exception& e) {
        project_.report_status(std::string("Error: apply failed: ") + e.what());
    }
}

void ConfigView::update_config_buttons() {
    bool has_config = !project_.config_path.empty();
    if (has_config) {
        auto markup = std::string("<b>Current Rex Config:</b> ") + project_.config_path.filename().string();
        gtk_label_set_markup(GTK_LABEL(config_label_), markup.c_str());
    } else {
        gtk_label_set_markup(GTK_LABEL(config_label_), "<b>Current Rex Config:</b> No config loaded");
    }
    gtk_widget_set_visible(btn_open_, !has_config);
    gtk_widget_set_visible(btn_create_, !has_config);
    gtk_widget_set_visible(btn_close_, has_config);
    gtk_widget_set_sensitive(config_content_, has_config);
}

void ConfigView::refresh() {
    build_config_fields();
    build_variables_section();
    update_resolved_labels();
    update_config_buttons();
    dirty_ = false;
    gtk_widget_remove_css_class(btn_save_, "suggested-action");
}

void ConfigView::on_open_config(GtkButton*, gpointer data) {
    auto* self = static_cast<ConfigView*>(data);
    auto* window = GTK_WINDOW(gtk_widget_get_ancestor(self->root_, GTK_TYPE_WINDOW));

    auto* dialog = gtk_file_dialog_new();
    gtk_file_dialog_set_title(dialog, "Open Rex Config");
    gtk_file_dialog_set_accept_label(dialog, "Select");

    auto* filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "Config files (*.config)");
    gtk_file_filter_add_pattern(filter, "*.config");
    auto* filters = g_list_store_new(GTK_TYPE_FILE_FILTER);
    g_list_store_append(filters, filter);
    g_object_unref(filter);
    gtk_file_dialog_set_filters(dialog, G_LIST_MODEL(filters));
    g_object_unref(filters);

    gtk_file_dialog_open(dialog, window, nullptr, on_open_config_response, self);
}

void ConfigView::on_open_config_response(GObject* source, GAsyncResult* res, gpointer data) {
    auto* self = static_cast<ConfigView*>(data);
    GError* error = nullptr;
    auto* file = gtk_file_dialog_open_finish(GTK_FILE_DIALOG(source), res, &error);
    if (!file) {
        if (error) g_error_free(error);
        return;
    }

    auto* path = g_file_get_path(file);
    g_object_unref(file);

    try {
        self->project_.open_config(path);
        self->refresh();
        if (self->apply_cb_)
            self->apply_cb_(self->apply_cb_data_);
    } catch (const std::exception& e) {
        self->project_.report_status(std::string("Error opening config: ") + e.what());
    }

    g_free(path);
}

void ConfigView::on_create_config(GtkButton*, gpointer data) {
    auto* self = static_cast<ConfigView*>(data);
    auto* window = GTK_WINDOW(gtk_widget_get_ancestor(self->root_, GTK_TYPE_WINDOW));

    auto* dialog = gtk_file_dialog_new();
    gtk_file_dialog_set_title(dialog, "Create Rex Config");
    gtk_file_dialog_set_initial_name(dialog, "rex.config");

    auto* filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "Config files (*.config)");
    gtk_file_filter_add_pattern(filter, "*.config");
    auto* filters = g_list_store_new(GTK_TYPE_FILE_FILTER);
    g_list_store_append(filters, filter);
    g_object_unref(filter);
    gtk_file_dialog_set_filters(dialog, G_LIST_MODEL(filters));
    g_object_unref(filters);

    gtk_file_dialog_save(dialog, window, nullptr, on_create_config_response, self);
}

void ConfigView::on_create_config_response(GObject* source, GAsyncResult* res, gpointer data) {
    auto* self = static_cast<ConfigView*>(data);
    GError* error = nullptr;
    auto* file = gtk_file_dialog_save_finish(GTK_FILE_DIALOG(source), res, &error);
    if (!file) {
        if (error) g_error_free(error);
        return;
    }

    auto* path = g_file_get_path(file);
    g_object_unref(file);

    try {
        // Write skeleton config
        nlohmann::json j;
        j["config"] = {
            {"config_version", "5"},
            {"project_root", ""},
            {"units_path", "units"},
            {"shells_path", ""},
            {"logs_path", ""}
        };
        std::filesystem::path fp(path);
        save_json_file(fp, j);

        self->project_.open_config(fp);
        self->refresh();
        if (self->apply_cb_)
            self->apply_cb_(self->apply_cb_data_);
    } catch (const std::exception& e) {
        self->project_.report_status(std::string("Error creating config: ") + e.what());
    }

    g_free(path);
}

void ConfigView::on_close_config(GtkButton*, gpointer data) {
    auto* self = static_cast<ConfigView*>(data);

    if (self->dirty_) {
        auto* parent = GTK_WINDOW(gtk_widget_get_ancestor(self->root_, GTK_TYPE_WINDOW));
        auto result = show_unsaved_dialog(parent);
        if (result == UnsavedResult::Cancel)
            return;
        if (result == UnsavedResult::Save)
            self->apply_config();
    }

    self->project_.close_config();
    self->refresh();
    if (self->apply_cb_)
        self->apply_cb_(self->apply_cb_data_);
}

}
