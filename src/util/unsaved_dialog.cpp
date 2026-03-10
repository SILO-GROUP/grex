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

namespace grex {

struct DialogState {
    UnsavedResult result;
    GMainLoop* loop;
    GtkWidget* window;
};

static void on_revert_clicked(GtkButton*, gpointer d) {
    auto* state = static_cast<DialogState*>(d);
    state->result = UnsavedResult::Revert;
    gtk_window_close(GTK_WINDOW(state->window));
}

static void on_save_clicked(GtkButton*, gpointer d) {
    auto* state = static_cast<DialogState*>(d);
    state->result = UnsavedResult::Save;
    gtk_window_close(GTK_WINDOW(state->window));
}

static void on_dialog_closed(GtkWidget*, gpointer d) {
    auto* state = static_cast<DialogState*>(d);
    g_main_loop_quit(state->loop);
}

UnsavedResult show_unsaved_dialog(GtkWindow* parent) {
    auto* win = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(win), "Unsaved Changes");
    gtk_window_set_transient_for(GTK_WINDOW(win), parent);
    gtk_window_set_modal(GTK_WINDOW(win), TRUE);
    gtk_window_set_default_size(GTK_WINDOW(win), 350, -1);
    gtk_window_set_resizable(GTK_WINDOW(win), FALSE);

    auto* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_margin_start(box, 16);
    gtk_widget_set_margin_end(box, 16);
    gtk_widget_set_margin_top(box, 16);
    gtk_widget_set_margin_bottom(box, 16);

    auto* label = gtk_label_new("You have unsaved changes.");
    gtk_box_append(GTK_BOX(box), label);

    auto* btn_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_widget_set_halign(btn_row, GTK_ALIGN_END);
    auto* btn_revert = gtk_button_new_with_label("Revert");
    auto* btn_save = gtk_button_new_with_label("Save");
    gtk_widget_add_css_class(btn_save, "suggested-action");
    gtk_box_append(GTK_BOX(btn_row), btn_revert);
    gtk_box_append(GTK_BOX(btn_row), btn_save);
    gtk_box_append(GTK_BOX(box), btn_row);

    gtk_window_set_child(GTK_WINDOW(win), box);
    gtk_window_set_default_widget(GTK_WINDOW(win), btn_save);

    auto* loop = g_main_loop_new(nullptr, FALSE);
    DialogState state{UnsavedResult::Save, loop, win};

    g_signal_connect(btn_revert, "clicked", G_CALLBACK(on_revert_clicked), &state);
    g_signal_connect(btn_save, "clicked", G_CALLBACK(on_save_clicked), &state);
    g_signal_connect(win, "destroy", G_CALLBACK(on_dialog_closed), &state);

    gtk_window_present(GTK_WINDOW(win));
    g_main_loop_run(loop);
    g_main_loop_unref(loop);

    return state.result;
}

}
