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

#include "util/unsaved_dialog.h"
#include <string>

namespace grex {

struct DialogState {
    UnsavedResult result;
    GMainLoop* loop;
    GtkWidget* window;
};

// When the parent gets WM focus while our dialog is up, yank it back.
static void on_parent_activated(GObject*, GParamSpec*, gpointer d) {
    auto* dialog_win = static_cast<GtkWidget*>(d);
    gtk_window_present(GTK_WINDOW(dialog_win));
}

static bool dialog_active = false;

UnsavedResult show_unsaved_dialog(GtkWindow* parent) {
    if (dialog_active)
        return UnsavedResult::Cancel;
    dialog_active = true;

    auto* win = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(win), "Unsaved Changes");
    gtk_window_set_transient_for(GTK_WINDOW(win), parent);
    gtk_window_set_modal(GTK_WINDOW(win), TRUE);
    gtk_window_set_default_size(GTK_WINDOW(win), 380, -1);
    gtk_window_set_resizable(GTK_WINDOW(win), FALSE);

    auto* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 16);
    gtk_widget_set_margin_start(box, 24);
    gtk_widget_set_margin_end(box, 24);
    gtk_widget_set_margin_top(box, 20);
    gtk_widget_set_margin_bottom(box, 20);

    // Header
    auto* header = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(header), "<big><b>Unsaved Changes</b></big>");
    gtk_widget_set_halign(header, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(box), header);

    // Description
    auto* desc = gtk_label_new("You have unsaved changes that will be lost if you continue without saving.");
    gtk_label_set_wrap(GTK_LABEL(desc), TRUE);
    gtk_label_set_xalign(GTK_LABEL(desc), 0.5f);
    gtk_widget_add_css_class(desc, "dim-label");
    gtk_box_append(GTK_BOX(box), desc);

    gtk_box_append(GTK_BOX(box), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

    // Button row
    auto* btn_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_halign(btn_row, GTK_ALIGN_END);

    auto* btn_discard = gtk_button_new_with_label("Discard");
    auto* btn_cancel = gtk_button_new_with_label("Cancel");
    auto* btn_save = gtk_button_new_with_label("Save");
    gtk_widget_add_css_class(btn_discard, "destructive-action");
    gtk_widget_add_css_class(btn_save, "suggested-action");
    gtk_box_append(GTK_BOX(btn_row), btn_discard);
    gtk_box_append(GTK_BOX(btn_row), btn_cancel);
    gtk_box_append(GTK_BOX(btn_row), btn_save);
    gtk_box_append(GTK_BOX(box), btn_row);

    gtk_window_set_child(GTK_WINDOW(win), box);
    gtk_window_set_default_widget(GTK_WINDOW(win), btn_save);

    auto* loop = g_main_loop_new(nullptr, FALSE);
    DialogState state{UnsavedResult::Cancel, loop, win};

    auto on_btn = +[](GtkButton* btn, gpointer d) {
        auto* s = static_cast<DialogState*>(d);
        auto label = std::string(gtk_button_get_label(btn));
        if (label == "Save") s->result = UnsavedResult::Save;
        else if (label == "Discard") s->result = UnsavedResult::Discard;
        else s->result = UnsavedResult::Cancel;
        gtk_window_close(GTK_WINDOW(s->window));
    };

    g_signal_connect(btn_discard, "clicked", G_CALLBACK(on_btn), &state);
    g_signal_connect(btn_cancel, "clicked", G_CALLBACK(on_btn), &state);
    g_signal_connect(btn_save, "clicked", G_CALLBACK(on_btn), &state);
    g_signal_connect(win, "destroy", G_CALLBACK(+[](GtkWidget*, gpointer d) {
        auto* s = static_cast<DialogState*>(d);
        g_main_loop_quit(s->loop);
    }), &state);

    // Steal focus back if WM raises the parent while dialog is up
    auto focus_guard = g_signal_connect(parent, "notify::is-active",
        G_CALLBACK(on_parent_activated), win);

    gtk_window_present(GTK_WINDOW(win));
    g_main_loop_run(loop);

    g_signal_handler_disconnect(parent, focus_guard);
    g_main_loop_unref(loop);
    dialog_active = false;

    return state.result;
}

}
