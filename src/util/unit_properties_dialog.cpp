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

#include "util/unit_properties_dialog.h"
#include <cstdlib>
#include <fstream>
#include <unistd.h>

namespace grex {

struct DialogState;

struct DlgSwitchBinding {
    DialogState* state;
    bool* target;
};

struct DlgEntryBinding {
    DialogState* state;
    std::string* target;
};

struct DialogState {
    UnitDialogResult result = UnitDialogResult::Cancel;
    GMainLoop* loop = nullptr;
    GtkWidget* window = nullptr;

    Unit working_copy;
    Unit* original = nullptr;
    Project* project = nullptr;
    GrexConfig* grex_config = nullptr;
    const std::vector<ShellDef>* shells = nullptr;

    bool loading = false;
    GCancellable* cancellable = nullptr;

    // widgets
    GtkWidget* entry_name = nullptr;
    GtkWidget* entry_target = nullptr;
    GtkWidget* box_target = nullptr;
    GtkWidget* switch_shell_cmd = nullptr;
    GtkWidget* dropdown_shell_def = nullptr;
    GtkWidget* switch_force_pty = nullptr;
    GtkWidget* switch_set_workdir = nullptr;
    GtkWidget* entry_workdir = nullptr;
    GtkWidget* box_workdir = nullptr;
    GtkWidget* switch_rectify = nullptr;
    GtkWidget* entry_rectifier = nullptr;
    GtkWidget* box_rectifier = nullptr;
    GtkWidget* switch_active = nullptr;
    GtkWidget* switch_required = nullptr;
    GtkWidget* switch_set_user_ctx = nullptr;
    GtkWidget* entry_user = nullptr;
    GtkWidget* entry_group = nullptr;
    GtkWidget* switch_supply_env = nullptr;
    GtkWidget* entry_environment = nullptr;
    GtkWidget* box_environment = nullptr;

    // labels for conditional visibility
    GtkWidget* label_target = nullptr;
    GtkWidget* label_shell_cmd = nullptr;
    GtkWidget* label_shell_def = nullptr;
    GtkWidget* label_force_pty = nullptr;
    GtkWidget* label_set_workdir = nullptr;
    GtkWidget* label_workdir = nullptr;
    GtkWidget* label_rectify = nullptr;
    GtkWidget* label_rectifier = nullptr;
    GtkWidget* label_active = nullptr;
    GtkWidget* label_required = nullptr;
    GtkWidget* label_set_user_ctx = nullptr;
    GtkWidget* label_user = nullptr;
    GtkWidget* label_group = nullptr;
    GtkWidget* label_supply_env = nullptr;
    GtkWidget* label_environment = nullptr;

    std::vector<DlgEntryBinding*> entry_bindings;
    std::vector<DlgSwitchBinding*> switch_bindings;
    std::vector<void*> helper_data;
};

static void update_sensitivity(DialogState* s);
static void validate_fields(DialogState* s);

static std::string extract_path(const std::string& s) {
    auto pos = s.find(' ');
    return pos == std::string::npos ? s : s.substr(0, pos);
}

static gboolean dlg_switch_state_set_cb(GtkSwitch* sw, gboolean new_state, gpointer data) {
    auto* b = static_cast<DlgSwitchBinding*>(data);
    gtk_switch_set_state(sw, new_state);
    if (b->state) {
        *b->target = new_state;
        if (!b->state->loading) {
            update_sensitivity(b->state);
            validate_fields(b->state);
        }
    }
    return TRUE;
}

static int shell_index(const std::vector<ShellDef>& shells, const std::string& name) {
    for (size_t i = 0; i < shells.size(); i++)
        if (shells[i].name == name) return static_cast<int>(i);
    return 0;
}

static GtkWidget* make_switch_row(GtkWidget* grid, int row, const char* label_text, GtkWidget** label_out) {
    auto* label = gtk_label_new(label_text);
    gtk_label_set_xalign(GTK_LABEL(label), 1.0f);
    gtk_widget_add_css_class(label, "dim-label");
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);
    if (label_out) *label_out = label;
    auto* sw = gtk_switch_new();
    gtk_widget_set_halign(sw, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), sw, 1, row, 1, 1);
    return sw;
}

static GtkWidget* make_entry_row(GtkWidget* grid, int row, const char* label_text, GtkWidget** label_out) {
    auto* label = gtk_label_new(label_text);
    gtk_label_set_xalign(GTK_LABEL(label), 1.0f);
    gtk_widget_add_css_class(label, "dim-label");
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);
    if (label_out) *label_out = label;
    auto* entry = gtk_entry_new();
    gtk_widget_set_hexpand(entry, TRUE);
    gtk_grid_attach(GTK_GRID(grid), entry, 1, row, 1, 1);
    return entry;
}

static GtkWidget* make_browse_row(DialogState* s, GtkWidget* grid, int row, const char* label_text,
                                   GtkWidget** label_out, GtkWidget** box_out) {
    auto* label = gtk_label_new(label_text);
    gtk_label_set_xalign(GTK_LABEL(label), 1.0f);
    gtk_widget_add_css_class(label, "dim-label");
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);
    if (label_out) *label_out = label;

    auto* hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_widget_set_hexpand(hbox, TRUE);

    auto* entry = gtk_entry_new();
    gtk_widget_set_hexpand(entry, TRUE);
    gtk_box_append(GTK_BOX(hbox), entry);

    auto* btn = gtk_button_new_with_label("Select...");
    gtk_box_append(GTK_BOX(hbox), btn);

    struct BrowseBtnData {
        DialogState* state;
        GtkWidget* entry;
    };
    auto* bbd = new BrowseBtnData{s, entry};
    s->helper_data.push_back(bbd);

    g_signal_connect(btn, "clicked", G_CALLBACK(+[](GtkButton*, gpointer d) {
        auto* bbd = static_cast<BrowseBtnData*>(d);
        auto* window = GTK_WINDOW(gtk_widget_get_ancestor(bbd->entry, GTK_TYPE_WINDOW));
        auto* dialog = gtk_file_dialog_new();
        gtk_file_dialog_set_title(dialog, "Select File");
        gtk_file_dialog_set_accept_label(dialog, "Select");

        auto root = bbd->state->project->resolved_project_root();
        if (!root.empty()) {
            auto* initial = g_file_new_for_path(root.c_str());
            gtk_file_dialog_set_initial_folder(dialog, initial);
            g_object_unref(initial);
        }

        struct BD { GtkWidget* entry; };
        auto* bd = new BD{bbd->entry};
        gtk_file_dialog_open(dialog, window, bbd->state->cancellable,
            +[](GObject* source, GAsyncResult* res, gpointer data) {
                auto* bd = static_cast<BD*>(data);
                GError* error = nullptr;
                auto* file = gtk_file_dialog_open_finish(GTK_FILE_DIALOG(source), res, &error);
                if (file) {
                    auto* path = g_file_get_path(file);
                    gtk_editable_set_text(GTK_EDITABLE(bd->entry), path);
                    g_free(path);
                    g_object_unref(file);
                } else if (error) {
                    g_error_free(error);
                }
                delete bd;
            }, bd);
    }), bbd);

    if (box_out) *box_out = hbox;
    gtk_grid_attach(GTK_GRID(grid), hbox, 1, row, 1, 1);
    return entry;
}

static GtkWidget* make_file_row(DialogState* s, GtkWidget* grid, int row, const char* label_text,
                                 GtkWidget** label_out, GtkWidget** box_out) {
    auto* label = gtk_label_new(label_text);
    gtk_label_set_xalign(GTK_LABEL(label), 1.0f);
    gtk_widget_add_css_class(label, "dim-label");
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);
    if (label_out) *label_out = label;

    auto* hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_widget_set_hexpand(hbox, TRUE);

    auto* entry = gtk_entry_new();
    gtk_widget_set_hexpand(entry, TRUE);
    gtk_box_append(GTK_BOX(hbox), entry);

    auto* file_btn_group = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_add_css_class(file_btn_group, "linked");
    auto* btn_browse = gtk_button_new_with_label("Select");
    auto* btn_open = gtk_button_new_with_label("Open");
    auto* btn_new = gtk_button_new_with_label("Create");
    gtk_box_append(GTK_BOX(file_btn_group), btn_browse);
    gtk_box_append(GTK_BOX(file_btn_group), btn_open);
    gtk_box_append(GTK_BOX(file_btn_group), btn_new);
    gtk_box_append(GTK_BOX(hbox), file_btn_group);

    struct FileBtnData {
        DialogState* state;
        GtkWidget* entry;
    };
    auto* fbd = new FileBtnData{s, entry};
    s->helper_data.push_back(fbd);

    // Browse
    g_signal_connect(btn_browse, "clicked", G_CALLBACK(+[](GtkButton*, gpointer d) {
        auto* fbd = static_cast<FileBtnData*>(d);
        auto* window = GTK_WINDOW(gtk_widget_get_ancestor(fbd->entry, GTK_TYPE_WINDOW));
        auto* dialog = gtk_file_dialog_new();
        gtk_file_dialog_set_title(dialog, "Select File");
        gtk_file_dialog_set_accept_label(dialog, "Select");

        auto root = fbd->state->project->resolved_project_root();
        if (!root.empty()) {
            auto* initial = g_file_new_for_path(root.c_str());
            gtk_file_dialog_set_initial_folder(dialog, initial);
            g_object_unref(initial);
        }

        struct CB { GtkWidget* entry; };
        auto* cb = new CB{fbd->entry};
        gtk_file_dialog_open(dialog, window, fbd->state->cancellable,
            +[](GObject* source, GAsyncResult* res, gpointer data) {
                auto* cb = static_cast<CB*>(data);
                GError* error = nullptr;
                auto* file = gtk_file_dialog_open_finish(GTK_FILE_DIALOG(source), res, &error);
                if (file) {
                    auto* path = g_file_get_path(file);
                    gtk_editable_set_text(GTK_EDITABLE(cb->entry), path);
                    g_free(path);
                    g_object_unref(file);
                } else if (error) {
                    g_error_free(error);
                }
                delete cb;
            }, cb);
    }), fbd);

    // Open
    g_signal_connect(btn_open, "clicked", G_CALLBACK(+[](GtkButton*, gpointer d) {
        auto* fbd = static_cast<FileBtnData*>(d);
        auto raw = std::string(gtk_editable_get_text(GTK_EDITABLE(fbd->entry)));
        if (raw.empty()) {
            fbd->state->project->report_status("Error: no file path set");
            return;
        }
        namespace fs = std::filesystem;
        fs::path p(extract_path(raw));
        if (p.is_relative()) {
            auto root = fbd->state->project->resolved_project_root();
            if (!root.empty())
                p = root / p;
        }
        std::error_code ec;
        auto canonical = fs::canonical(p, ec);
        auto full = ec ? p : canonical;
        if (!fs::exists(full)) {
            fbd->state->project->report_status("Error: file not found: " + full.string());
            return;
        }
        auto cmd = fbd->state->grex_config->file_editor + " \"" + full.string() + "\" &";
        std::system(cmd.c_str());
    }), fbd);

    // Create
    auto on_new_clicked = +[](GtkButton*, gpointer d) {
        auto* fbd = static_cast<FileBtnData*>(d);
        auto* window = GTK_WINDOW(gtk_widget_get_ancestor(fbd->entry, GTK_TYPE_WINDOW));
        auto* dialog = gtk_file_dialog_new();
        gtk_file_dialog_set_title(dialog, "Create New File");

        auto root = fbd->state->project->resolved_project_root();
        if (!root.empty()) {
            auto* initial = g_file_new_for_path(root.c_str());
            gtk_file_dialog_set_initial_folder(dialog, initial);
            g_object_unref(initial);
        }

        struct NewCB {
            DialogState* state;
            GtkWidget* entry;
        };
        auto* ncb = new NewCB{fbd->state, fbd->entry};
        auto on_save_response = +[](GObject* source, GAsyncResult* res, gpointer data) {
            auto* ncb = static_cast<NewCB*>(data);
            GError* error = nullptr;
            auto* file = gtk_file_dialog_save_finish(GTK_FILE_DIALOG(source), res, &error);
            if (file) {
                auto* path = g_file_get_path(file);
                std::ofstream ofs(path);
                ofs.close();
                gtk_editable_set_text(GTK_EDITABLE(ncb->entry), path);
                auto cmd = ncb->state->grex_config->file_editor + " \"" + std::string(path) + "\" &";
                std::system(cmd.c_str());
                g_free(path);
                g_object_unref(file);
            } else if (error) {
                g_error_free(error);
            }
            delete ncb;
        };
        gtk_file_dialog_save(dialog, window, fbd->state->cancellable, on_save_response, ncb);
    };
    g_signal_connect(btn_new, "clicked", G_CALLBACK(on_new_clicked), fbd);

    if (box_out) *box_out = hbox;
    gtk_grid_attach(GTK_GRID(grid), hbox, 1, row, 1, 1);
    return entry;
}

static void update_sensitivity(DialogState* s) {
    auto show = [](bool visible, std::initializer_list<GtkWidget*> widgets) {
        for (auto* w : widgets)
            gtk_widget_set_visible(w, visible);
    };

    bool active = s->working_copy.active;

    show(active, {
        s->label_target, s->box_target,
        s->label_shell_cmd, s->switch_shell_cmd,
        s->label_set_workdir, s->switch_set_workdir,
        s->label_rectify, s->switch_rectify,
        s->label_required, s->switch_required,
        s->label_set_user_ctx, s->switch_set_user_ctx,
        s->label_supply_env, s->switch_supply_env,
    });

    show(active && s->working_copy.is_shell_command, {s->label_shell_def, s->dropdown_shell_def, s->label_force_pty, s->switch_force_pty});
    show(active && s->working_copy.set_working_directory, {s->label_workdir, s->box_workdir});
    show(active && s->working_copy.rectify, {s->label_rectifier, s->box_rectifier});
    show(active && s->working_copy.set_user_context, {s->label_user, s->entry_user, s->label_group, s->entry_group});
    show(active && s->working_copy.supply_environment, {s->label_environment, s->box_environment});
}

static void validate_fields(DialogState* s) {
    if (s->loading) return;

    auto& u = s->working_copy;
    namespace fs = std::filesystem;

    auto set_valid = [](GtkWidget* w, bool valid) {
        if (valid)
            gtk_widget_remove_css_class(w, "error");
        else
            gtk_widget_add_css_class(w, "error");
    };

    std::string error_msg;

    // Target: must be defined, exist, and be executable
    {
        bool valid = true;
        if (u.target.empty()) {
            valid = false;
            if (error_msg.empty()) error_msg = "Target is not defined";
        } else {
            auto tp = extract_path(u.target);
            if (!fs::exists(tp)) {
                valid = false;
                if (error_msg.empty()) error_msg = "Target does not exist: " + tp;
            } else if (access(tp.c_str(), X_OK) != 0) {
                valid = false;
                if (error_msg.empty()) error_msg = "Target is not executable: " + tp;
            }
        }
        set_valid(s->entry_target, valid);
    }

    // Shell definition: if shell command, must reference a known shell
    if (u.is_shell_command) {
        bool valid = false;
        for (auto& sh : *s->shells) {
            if (sh.name == u.shell_definition) { valid = true; break; }
        }
        set_valid(s->dropdown_shell_def, valid);
        if (!valid && error_msg.empty())
            error_msg = "Shell definition not found: " + u.shell_definition;
    } else {
        set_valid(s->dropdown_shell_def, true);
    }

    // Working directory: must exist as a directory if enabled
    if (u.set_working_directory) {
        bool valid = true;
        if (u.working_directory.empty()) {
            valid = false;
            if (error_msg.empty()) error_msg = "Working directory is not defined";
        } else if (!fs::is_directory(u.working_directory)) {
            valid = false;
            if (error_msg.empty()) error_msg = "Working directory does not exist: " + u.working_directory;
        }
        set_valid(s->entry_workdir, valid);
    } else {
        set_valid(s->entry_workdir, true);
    }

    // Rectifier: must exist and be executable if rectification enabled
    if (u.rectify) {
        bool valid = true;
        if (u.rectifier.empty()) {
            valid = false;
            if (error_msg.empty()) error_msg = "Rectifier is not defined";
        } else {
            auto rp = extract_path(u.rectifier);
            if (!fs::exists(rp)) {
                valid = false;
                if (error_msg.empty()) error_msg = "Rectifier does not exist: " + rp;
            } else if (access(rp.c_str(), X_OK) != 0) {
                valid = false;
                if (error_msg.empty()) error_msg = "Rectifier is not executable: " + rp;
            }
        }
        set_valid(s->entry_rectifier, valid);
    } else {
        set_valid(s->entry_rectifier, true);
    }

    // Environment file: must exist if supply_environment active
    if (u.supply_environment) {
        bool valid = true;
        if (u.environment.empty()) {
            valid = false;
            if (error_msg.empty()) error_msg = "Environment file is not defined";
        } else {
            auto ep = extract_path(u.environment);
            if (!fs::exists(ep)) {
                valid = false;
                if (error_msg.empty()) error_msg = "Environment file does not exist: " + ep;
            }
        }
        set_valid(s->entry_environment, valid);
    } else {
        set_valid(s->entry_environment, true);
    }

    if (!error_msg.empty())
        s->project->report_status(error_msg);
}

static void populate_and_connect(DialogState* s) {
    auto& u = s->working_copy;

    // Rebuild shell dropdown model
    auto* string_list = gtk_string_list_new(nullptr);
    for (auto& sh : *s->shells)
        gtk_string_list_append(string_list, sh.name.c_str());
    gtk_drop_down_set_model(GTK_DROP_DOWN(s->dropdown_shell_def), G_LIST_MODEL(string_list));
    g_object_unref(string_list);

    s->loading = true;
    gtk_editable_set_text(GTK_EDITABLE(s->entry_name), u.name.c_str());
    gtk_editable_set_text(GTK_EDITABLE(s->entry_target), u.target.c_str());
    gtk_switch_set_active(GTK_SWITCH(s->switch_shell_cmd), u.is_shell_command);
    gtk_drop_down_set_selected(GTK_DROP_DOWN(s->dropdown_shell_def), shell_index(*s->shells, u.shell_definition));
    gtk_switch_set_active(GTK_SWITCH(s->switch_force_pty), u.force_pty);
    gtk_switch_set_active(GTK_SWITCH(s->switch_set_workdir), u.set_working_directory);
    gtk_editable_set_text(GTK_EDITABLE(s->entry_workdir), u.working_directory.c_str());
    gtk_switch_set_active(GTK_SWITCH(s->switch_rectify), u.rectify);
    gtk_editable_set_text(GTK_EDITABLE(s->entry_rectifier), u.rectifier.c_str());
    gtk_switch_set_active(GTK_SWITCH(s->switch_active), u.active);
    gtk_switch_set_active(GTK_SWITCH(s->switch_required), u.required);
    gtk_switch_set_active(GTK_SWITCH(s->switch_set_user_ctx), u.set_user_context);
    gtk_editable_set_text(GTK_EDITABLE(s->entry_user), u.user.c_str());
    gtk_editable_set_text(GTK_EDITABLE(s->entry_group), u.group.c_str());
    gtk_switch_set_active(GTK_SWITCH(s->switch_supply_env), u.supply_environment);
    gtk_editable_set_text(GTK_EDITABLE(s->entry_environment), u.environment.c_str());
    update_sensitivity(s);
    s->loading = false;
    validate_fields(s);

    // Entry bindings
    auto bind_entry = [s](GtkWidget* entry, std::string* target) {
        auto* eb = new DlgEntryBinding{s, target};
        s->entry_bindings.push_back(eb);
        g_signal_connect(entry, "changed", G_CALLBACK(+[](GtkEditable* e, gpointer d) {
            auto* eb = static_cast<DlgEntryBinding*>(d);
            *eb->target = gtk_editable_get_text(e);
            validate_fields(eb->state);
        }), eb);
    };
    bind_entry(s->entry_name, &u.name);
    bind_entry(s->entry_target, &u.target);
    bind_entry(s->entry_workdir, &u.working_directory);
    bind_entry(s->entry_rectifier, &u.rectifier);
    bind_entry(s->entry_user, &u.user);
    bind_entry(s->entry_group, &u.group);
    bind_entry(s->entry_environment, &u.environment);

    // Switch bindings
    auto bind_switch = [s](GtkWidget* sw, bool* target) {
        auto* sb = new DlgSwitchBinding{s, target};
        s->switch_bindings.push_back(sb);
        g_signal_connect(sw, "state-set", G_CALLBACK(dlg_switch_state_set_cb), sb);
    };
    bind_switch(s->switch_shell_cmd, &u.is_shell_command);
    bind_switch(s->switch_force_pty, &u.force_pty);
    bind_switch(s->switch_set_workdir, &u.set_working_directory);
    bind_switch(s->switch_rectify, &u.rectify);
    bind_switch(s->switch_active, &u.active);
    bind_switch(s->switch_required, &u.required);
    bind_switch(s->switch_set_user_ctx, &u.set_user_context);
    bind_switch(s->switch_supply_env, &u.supply_environment);

    // Shell dropdown
    g_signal_connect(s->dropdown_shell_def, "notify::selected", G_CALLBACK(+[](GObject* obj, GParamSpec*, gpointer d) {
        auto* s = static_cast<DialogState*>(d);
        if (s->loading) return;
        auto idx = gtk_drop_down_get_selected(GTK_DROP_DOWN(obj));
        if (idx < s->shells->size())
            s->working_copy.shell_definition = (*s->shells)[idx].name;
        validate_fields(s);
    }), s);
}

UnitDialogResult show_unit_properties_dialog(GtkWindow* parent,
    Unit* unit, Project& project, GrexConfig& grex_config,
    const std::vector<ShellDef>& shells)
{
    auto* win = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(win), ("Unit Properties: " + unit->name).c_str());
    gtk_window_set_transient_for(GTK_WINDOW(win), parent);
    gtk_window_set_modal(GTK_WINDOW(win), TRUE);
    gtk_window_set_default_size(GTK_WINDOW(win), 650, 620);

    auto* outer_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_margin_start(outer_box, 20);
    gtk_widget_set_margin_end(outer_box, 20);
    gtk_widget_set_margin_top(outer_box, 16);
    gtk_widget_set_margin_bottom(outer_box, 16);

    // Scrolled content area
    auto* scroll = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_vexpand(scroll, TRUE);

    auto* content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_start(content, 4);
    gtk_widget_set_margin_end(content, 4);
    gtk_widget_set_margin_top(content, 8);
    gtk_widget_set_margin_bottom(content, 8);

    auto* loop = g_main_loop_new(nullptr, FALSE);
    auto* cancellable = g_cancellable_new();
    DialogState state{};
    state.loop = loop;
    state.window = win;
    state.working_copy = *unit;
    state.original = unit;
    state.project = &project;
    state.grex_config = &grex_config;
    state.shells = &shells;
    state.cancellable = cancellable;

    // Helper to create a framed section with a bold title label
    auto make_section = [&](const char* title) {
        auto* frame = gtk_frame_new(nullptr);
        auto* frame_label = gtk_label_new(nullptr);
        gtk_label_set_markup(GTK_LABEL(frame_label), (std::string(" <b>") + title + "</b> ").c_str());
        gtk_frame_set_label_widget(GTK_FRAME(frame), frame_label);
        gtk_box_append(GTK_BOX(content), frame);
        return frame;
    };

    // Helper to create a grid inside a frame with internal padding
    auto make_grid = [&](GtkWidget* frame) {
        auto* grid = gtk_grid_new();
        gtk_grid_set_row_spacing(GTK_GRID(grid), 8);
        gtk_grid_set_column_spacing(GTK_GRID(grid), 12);
        gtk_widget_set_margin_start(grid, 12);
        gtk_widget_set_margin_end(grid, 12);
        gtk_widget_set_margin_top(grid, 8);
        gtk_widget_set_margin_bottom(grid, 12);
        gtk_frame_set_child(GTK_FRAME(frame), grid);
        return grid;
    };

    // === Identity ===
    auto* id_frame = make_section("Identity");
    auto* id_grid = make_grid(id_frame);
    state.entry_name = make_entry_row(id_grid, 0, "Name", nullptr);

    // === Status ===
    auto* status_frame = make_section("Status");
    auto* status_grid = make_grid(status_frame);
    state.switch_active = make_switch_row(status_grid, 0, "Active", &state.label_active);
    state.switch_required = make_switch_row(status_grid, 1, "Required", &state.label_required);

    // === Execution ===
    auto* exec_frame = make_section("Execution");
    auto* exec_grid = make_grid(exec_frame);
    int r = 0;
    state.entry_target = make_file_row(&state, exec_grid, r++, "Target", &state.label_target, &state.box_target);
    state.switch_shell_cmd = make_switch_row(exec_grid, r++, "Shell Command", &state.label_shell_cmd);

    state.label_shell_def = gtk_label_new("Shell Definition");
    gtk_label_set_xalign(GTK_LABEL(state.label_shell_def), 1.0f);
    gtk_widget_add_css_class(state.label_shell_def, "dim-label");
    gtk_grid_attach(GTK_GRID(exec_grid), state.label_shell_def, 0, r, 1, 1);
    auto* string_list = gtk_string_list_new(nullptr);
    state.dropdown_shell_def = gtk_drop_down_new(G_LIST_MODEL(string_list), nullptr);
    gtk_widget_set_hexpand(state.dropdown_shell_def, TRUE);
    gtk_grid_attach(GTK_GRID(exec_grid), state.dropdown_shell_def, 1, r++, 1, 1);

    state.switch_force_pty = make_switch_row(exec_grid, r++, "Force PTY", &state.label_force_pty);

    // === Working Directory ===
    auto* wd_frame = make_section("Working Directory");
    auto* wd_grid = make_grid(wd_frame);
    state.switch_set_workdir = make_switch_row(wd_grid, 0, "Set Working Dir", &state.label_set_workdir);
    state.entry_workdir = make_browse_row(&state, wd_grid, 1, "Working Directory", &state.label_workdir, &state.box_workdir);

    // === Validation ===
    auto* val_frame = make_section("Validation");
    auto* val_grid = make_grid(val_frame);
    state.switch_rectify = make_switch_row(val_grid, 0, "Rectify", &state.label_rectify);
    state.entry_rectifier = make_file_row(&state, val_grid, 1, "Rectifier", &state.label_rectifier, &state.box_rectifier);

    // === Security ===
    auto* sec_frame = make_section("Security");
    auto* sec_grid = make_grid(sec_frame);
    state.switch_set_user_ctx = make_switch_row(sec_grid, 0, "Set User Context", &state.label_set_user_ctx);
    state.entry_user = make_entry_row(sec_grid, 1, "User", &state.label_user);
    state.entry_group = make_entry_row(sec_grid, 2, "Group", &state.label_group);

    // === Environment ===
    auto* env_frame = make_section("Environment");
    auto* env_grid = make_grid(env_frame);
    state.switch_supply_env = make_switch_row(env_grid, 0, "Supply Environment", &state.label_supply_env);
    state.entry_environment = make_file_row(&state, env_grid, 1, "Environment", &state.label_environment, &state.box_environment);

    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), content);
    gtk_box_append(GTK_BOX(outer_box), scroll);

    // Button row
    auto* btn_sep = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_widget_set_margin_top(btn_sep, 4);
    gtk_box_append(GTK_BOX(outer_box), btn_sep);
    auto* btn_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_halign(btn_row, GTK_ALIGN_END);
    gtk_widget_set_margin_top(btn_row, 4);
    auto* btn_cancel = gtk_button_new_with_label("Cancel");
    auto* btn_save = gtk_button_new_with_label("Save");
    gtk_widget_add_css_class(btn_save, "suggested-action");
    gtk_box_append(GTK_BOX(btn_row), btn_cancel);
    gtk_box_append(GTK_BOX(btn_row), btn_save);
    gtk_box_append(GTK_BOX(outer_box), btn_row);

    gtk_window_set_child(GTK_WINDOW(win), outer_box);
    gtk_window_set_default_widget(GTK_WINDOW(win), btn_save);

    // Populate widgets and connect signals
    populate_and_connect(&state);

    // Button signals
    g_signal_connect(btn_cancel, "clicked", G_CALLBACK(+[](GtkButton*, gpointer d) {
        auto* s = static_cast<DialogState*>(d);
        s->result = UnitDialogResult::Cancel;
        gtk_window_close(GTK_WINDOW(s->window));
    }), &state);

    g_signal_connect(btn_save, "clicked", G_CALLBACK(+[](GtkButton*, gpointer d) {
        auto* s = static_cast<DialogState*>(d);
        if (s->working_copy.name.empty()) {
            s->project->report_status("Error: unit name cannot be empty");
            return;
        }
        if (s->working_copy.name != s->original->name &&
            s->project->is_unit_name_taken(s->working_copy.name, s->original)) {
            s->project->report_status("Error: unit '" + s->working_copy.name + "' already exists");
            return;
        }
        *s->original = s->working_copy;
        s->result = UnitDialogResult::Save;
        gtk_window_close(GTK_WINDOW(s->window));
    }), &state);

    g_signal_connect(win, "destroy", G_CALLBACK(+[](GtkWidget*, gpointer d) {
        auto* s = static_cast<DialogState*>(d);
        g_main_loop_quit(s->loop);
    }), &state);

    gtk_window_present(GTK_WINDOW(win));
    g_main_loop_run(loop);
    g_main_loop_unref(loop);

    // Cleanup
    g_cancellable_cancel(cancellable);
    g_object_unref(cancellable);
    for (auto* eb : state.entry_bindings) delete eb;
    for (auto* sb : state.switch_bindings) delete sb;
    for (auto* p : state.helper_data) ::operator delete(p);

    return state.result;
}

}
