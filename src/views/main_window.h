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

#pragma once
#include <gtk/gtk.h>
#include <string>
#include <vector>
#include "models/project.h"
#include "models/grex_config.h"

namespace grex {

class ConfigView;
class PlanView;
class UnitsView;
class ShellsView;
class MainWindow {
public:
    MainWindow(GtkApplication* app, Project& project, GrexConfig& grex_config);
    ~MainWindow();
    GtkWidget* widget() { return window_; }
    void set_status(const std::string& msg);

private:
    Project& project_;
    GrexConfig& grex_config_;
    GtkWidget* window_;
    ConfigView* config_view_;
    PlanView* plan_view_;
    UnitsView* units_view_;
    ShellsView* shells_view_;

    GtkWidget* stack_;
    GtkWidget* status_label_;

    struct TabInfo {
        const char* name;
        GtkWidget* page;
        GtkWidget* button;
    };
    std::vector<TabInfo> tabs_;
    int current_tab_ = 0;

    void update_tab_sensitivity();
    void refresh_visible_page();
    bool check_dirty_and_resolve();
    void switch_to_tab(int idx);
    void update_tab_buttons();

    static void on_config_applied(void* data);
    static void on_project_status(const std::string& msg, void* data);
    static void on_grex_config_clicked(GtkButton* btn, gpointer data);
};

}
