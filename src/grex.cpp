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

#include <gtk/gtk.h>
#include <iostream>
#include <string>
#include "models/project.h"
#include "models/grex_config.h"
#include "views/main_window.h"

static grex::Project* g_project = nullptr;
static grex::GrexConfig* g_grex_config = nullptr;
static grex::MainWindow* g_main_window = nullptr;
static std::string g_config_path;

static int on_command_line(GApplication* app, GApplicationCommandLine* cmdline, gpointer) {
    GVariantDict* options = g_application_command_line_get_options_dict(cmdline);

    const gchar* config_path = nullptr;
    g_variant_dict_lookup(options, "config", "&s", &config_path);
    if (config_path)
        g_config_path = config_path;

    g_application_activate(app);
    return 0;
}

static void on_activate(GtkApplication* app, gpointer) {
    if (g_main_window) {
        gtk_window_present(GTK_WINDOW(g_main_window->widget()));
        return;
    }

    try {
        g_grex_config = new grex::GrexConfig(grex::GrexConfig::load());
        if (!g_config_path.empty()) {
            g_project = new grex::Project(grex::Project::load(g_config_path));
        } else {
            g_project = new grex::Project();
        }
        g_main_window = new grex::MainWindow(app, *g_project, *g_grex_config);
        gtk_window_present(GTK_WINDOW(g_main_window->widget()));
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;

        auto* dialog = gtk_alert_dialog_new("Failed to start:\n%s", e.what());
        gtk_alert_dialog_show(dialog, nullptr);
        g_object_unref(dialog);
    }
}

int main(int argc, char* argv[]) {
    auto* app = gtk_application_new("org.darkhorselinux.grex", G_APPLICATION_HANDLES_COMMAND_LINE);

    GOptionEntry entries[] = {
        {"config", 'c', 0, G_OPTION_ARG_STRING, nullptr, "Path to rex.config", "FILE"},
        {nullptr}
    };
    g_application_add_main_option_entries(G_APPLICATION(app), entries);

    g_signal_connect(app, "command-line", G_CALLBACK(on_command_line), nullptr);
    g_signal_connect(app, "activate", G_CALLBACK(on_activate), nullptr);

    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    delete g_main_window;
    delete g_project;
    delete g_grex_config;

    return status;
}
