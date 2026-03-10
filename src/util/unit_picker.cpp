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

#include "util/unit_picker.h"
#include <cstring>

static int sort_listbox_alpha(GtkListBoxRow* a, GtkListBoxRow* b, gpointer) {
    auto* la = GTK_LABEL(gtk_list_box_row_get_child(GTK_LIST_BOX_ROW(a)));
    auto* lb = GTK_LABEL(gtk_list_box_row_get_child(GTK_LIST_BOX_ROW(b)));
    return std::strcmp(gtk_label_get_text(la), gtk_label_get_text(lb));
}

namespace grex {

struct PickerData {
    std::function<void(const std::string&)> on_select;
    GtkWidget* listbox;
    GtkWidget* window;
};

static std::string extract_unit_name(GtkListBoxRow* row) {
    auto* label = gtk_list_box_row_get_child(GTK_LIST_BOX_ROW(row));
    auto name = std::string(gtk_label_get_text(GTK_LABEL(label)));
    if (name.rfind("\u25C6 ", 0) == 0) name = name.substr(strlen("\u25C6 "));
    return name;
}

void show_unit_picker(GtkWindow* parent, Project& project,
    std::function<void(const std::string& unit_name)> on_select) {

    auto* win = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(win), "Defined Units");
    gtk_window_set_transient_for(GTK_WINDOW(win), parent);
    gtk_window_set_modal(GTK_WINDOW(win), TRUE);
    gtk_window_set_default_size(GTK_WINDOW(win), 350, 400);

    auto* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_widget_set_margin_start(box, 8);
    gtk_widget_set_margin_end(box, 8);
    gtk_widget_set_margin_top(box, 8);
    gtk_widget_set_margin_bottom(box, 8);

    auto* header = gtk_label_new("Select a defined unit...");
    gtk_label_set_xalign(GTK_LABEL(header), 0.0f);
    gtk_box_append(GTK_BOX(box), header);

    auto* scroll = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_vexpand(scroll, TRUE);

    auto* listbox = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(listbox), GTK_SELECTION_SINGLE);
    gtk_list_box_set_sort_func(GTK_LIST_BOX(listbox), sort_listbox_alpha, nullptr, nullptr);

    for (auto& uf : project.unit_files) {
        for (auto& u : uf.units) {
            auto* row = gtk_list_box_row_new();
            auto utext = std::string("\u25C6 ") + u.name;
            auto* label = gtk_label_new(utext.c_str());
            gtk_label_set_xalign(GTK_LABEL(label), 0.0f);
            gtk_widget_set_margin_start(label, 8);
            gtk_widget_set_margin_end(label, 8);
            gtk_widget_set_margin_top(label, 4);
            gtk_widget_set_margin_bottom(label, 4);
            gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), label);
            gtk_list_box_append(GTK_LIST_BOX(listbox), row);
        }
    }

    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), listbox);
    gtk_box_append(GTK_BOX(box), scroll);

    auto* btn_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_widget_set_halign(btn_row, GTK_ALIGN_END);
    auto* btn_cancel = gtk_button_new_with_label("Cancel");
    auto* btn_select = gtk_button_new_with_label("Select");
    gtk_box_append(GTK_BOX(btn_row), btn_cancel);
    gtk_box_append(GTK_BOX(btn_row), btn_select);
    gtk_box_append(GTK_BOX(box), btn_row);

    gtk_window_set_child(GTK_WINDOW(win), box);

    auto* pd = new PickerData{std::move(on_select), listbox, win};

    g_signal_connect(btn_cancel, "clicked", G_CALLBACK(+[](GtkButton*, gpointer d) {
        auto* pd = static_cast<PickerData*>(d);
        gtk_window_close(GTK_WINDOW(pd->window));
        delete pd;
    }), pd);

    g_signal_connect(btn_select, "clicked", G_CALLBACK(+[](GtkButton*, gpointer d) {
        auto* pd = static_cast<PickerData*>(d);
        auto* row = gtk_list_box_get_selected_row(GTK_LIST_BOX(pd->listbox));
        if (!row) return;
        pd->on_select(extract_unit_name(row));
        gtk_window_close(GTK_WINDOW(pd->window));
        delete pd;
    }), pd);

    g_signal_connect(listbox, "row-activated", G_CALLBACK(+[](GtkListBox*, GtkListBoxRow* row, gpointer d) {
        auto* pd = static_cast<PickerData*>(d);
        pd->on_select(extract_unit_name(row));
        gtk_window_close(GTK_WINDOW(pd->window));
        delete pd;
    }), pd);

    gtk_window_present(GTK_WINDOW(win));
}

}
