# GREX

**A graphical editor for Rex projects.**

AGPL-3.0 | Written by Chris Punches | [SILO GROUP](https://www.silogroup.org)

---

## What is GREX?

GREX is a desktop application for creating and managing
[Rex](https://git.silogroup.org/SILO-GROUP/rex) project files. Rex is SILO
GROUP's JSON-driven workflow execution system — it runs scripts and executables
in a predetermined order with logging, error handling, and self-healing
capabilities. Rex was originally built to drive the creation of
[Dark Horse Linux](https://www.darkhorselinux.org) but is designed for any
automation use case.

Rex projects are configured entirely through JSON files: configs, plans, unit
definitions, and shell definitions. GREX provides a visual interface for
managing all of these, replacing the need to hand-edit JSON. GREX edits Rex
project files — it does not execute them. Rex remains the runtime.

---

## Installation

### Prerequisites

- A C++17-capable compiler (GCC 8+ or Clang 7+)
- CMake 3.10 or later
- GTK4 development libraries
- pkg-config

The nlohmann/json library is vendored and does not require separate
installation.

### Building

```bash
cmake -B build
cmake --build build
```

The resulting binary is `build/grex`.

### Distribution Packages

Distribution-specific packages are not yet available. To install system-wide,
place the `grex` binary in a location on your `PATH` (e.g.,
`/usr/local/bin`).

---

## Getting Started

Launch GREX with no arguments to start with an empty session:

```bash
grex
```

Or open directly into a Rex project by passing a config file:

```bash
grex -c /path/to/rex.config
```

When you launch without a config, the first thing to do is open or create one
on the **Rex Config** tab. Once a config is loaded and its paths resolve
correctly, the **Plans**, **Units**, and **Shells** tabs become active.

On first launch, GREX creates its own settings file at
`~/.config/grex/grex.ini` with default values.

A status bar at the bottom of the window reports the results of load, save, and
error operations as you work.

---

## Working with Rex Configs

The **Rex Config** tab is where you load and manage your project's
configuration file.

### Opening and Creating Configs

- **Open Config** — Browse to an existing `rex.config` file.
- **Create Config** — Choose a location and filename to create a new, empty
  config.
- **Close Config** — Unload the current config. The other tabs will be
  disabled until a new config is loaded.

### Editing Configuration Values

Once a config is loaded, its key-value pairs are displayed as editable fields.
The standard keys are:

- `project_root` — The root directory of your Rex project.
- `units_path` — Path to the directory containing `.units` files (relative to
  project root).
- `shells_path` — Path to the shells definition file (relative to project
  root).

You can also define custom keys for your own use.

### Variables

Configuration values can contain variable references using `${VAR}` syntax.
For example:

```
project_root = ${HOME}/my-rex-project
```

GREX resolves these variables from your environment. If a variable is not set
in your environment, the **Variables** section will show it as unresolved and
let you provide an override value manually.

### Resolved Paths

The **Resolved Paths** section shows the fully-expanded values of
`project_root`, `units_dir`, and `shells_path` after variable substitution.
Check this section to confirm that your paths are pointing where you expect
before moving on to the other tabs.

### Saving

Click **Save** to write the config back to disk. The save button turns blue
when there are unsaved changes.

---

## Working with Plans

The **Plans** tab is where you create and manage execution plans — the ordered
sequences of tasks that Rex runs.

### Opening and Creating Plans

- **Open Plan** — Browse to an existing plan file.
- **Create Plan** — Choose a location and filename for a new, empty plan.
- **Close Plan** — Unload the current plan.

The current plan filename is shown in the header.

### Managing Tasks

The left panel lists the tasks in the current plan. Tasks are executed by Rex
in the order they appear.

- **Add** — Append a new empty task to the plan.
- **Delete** — Remove the selected task.
- **Move Up / Move Down** — Reorder the selected task.

### Editing a Task

Select a task in the list to view its properties in the right panel:

- **Name** — The name of the unit this task will execute (read-only display;
  change it with the unit picker below).
- **Comment** — An optional note describing what this task does or why it
  exists.
- **Change/Select Unit...** — Opens a picker dialog listing all units across
  your loaded unit files. Select one to assign it to this task.
- **Edit Unit...** — Opens a modal dialog to edit the properties of the unit
  assigned to this task.
- **Save Unit** — Writes changes to the unit's file on disk. Turns blue when
  the unit has unsaved changes.

### Saving the Plan

Click **Save Plan** to write the plan file to disk. The button turns blue when
there are unsaved changes. If you switch tabs with unsaved changes, GREX will
prompt you to save or revert.

---

## Working with Units

The **Units** tab is where you manage unit definitions — the building blocks
that Rex executes.

### Browsing Unit Files

The left panel lists all `.units` files found in your project's units
directory. Select a file to see its units in the right panel.

### Managing Unit Files

- **New File** — Create a new `.units` file in your units directory.
- **Delete File** — Remove the selected file from the project (the file is not
  deleted from disk).

### Managing Units Within a File

The right panel lists the units in the selected file, in the order they appear
in the file.

- **New Unit** — Add a new unit to the current file. You will be prompted for
  a name. Unit names must be unique across all unit files in the project.
- **Delete Unit** — Remove the selected unit.
- **Edit Unit...** — Open a modal dialog to edit the selected unit's
  properties.
- **Move Up / Move Down** — Reorder the selected unit within the file.
- **Rename** — Double-click a unit to rename it inline. Press Enter to confirm
  or click elsewhere to cancel.

### Unit Properties

When you open the unit properties dialog (from either the Units or Plans tab),
you can configure:

- **Target** — The script or command that Rex will execute.
- **Shell Definition** — Which shell interpreter to use (selected from your
  project's shell definitions).
- **Shell Command** — Whether the target is a shell command (interpreted by the
  shell) or a direct path to an executable.
- **Force PTY** — Allocate a pseudo-terminal for execution.
- **Rectify / Rectifier** — Enable self-healing behavior. If the target fails
  and rectification is enabled, Rex runs the rectifier script before
  continuing. This is one of Rex's most powerful features — it allows
  verification-and-fix patterns and preventive error recovery.
- **Active** — Whether Rex will include this unit when executing a plan. Inactive
  units are skipped.
- **Required** — Whether failure of this unit should halt plan execution. If a
  non-required unit fails (and rectification doesn't resolve it), Rex
  continues to the next task.
- **User / Group** — Run the target under a specific user and group identity.
- **Working Directory** — Set a custom working directory for execution.
- **Environment** — Path to an environment file to source before execution.

You can also browse for file paths and open or create files in your configured
editor directly from the dialog.

### Saving

Click **Save Unit File** to write changes to disk. The button turns blue when
there are unsaved changes. Switching to a different unit file with unsaved
changes will prompt you to save or revert.

---

## Working with Shells

The **Shells** tab is where you manage shell definitions — the interpreters
that Rex uses to run unit targets.

### Managing Shell Definitions

The left panel lists all defined shells. Select one to edit its properties in
the right panel.

- **Add** — Create a new shell definition.
- **Delete** — Remove the selected shell definition.

### Shell Properties

Each shell definition has four fields:

- **Name** — An identifier for the shell (e.g., `bash`, `zsh`, `python3`).
  This is the name referenced by units' shell definition setting.
- **Path** — The full filesystem path to the interpreter binary (e.g.,
  `/bin/bash`).
- **Execution Arg** — The argument used to pass a command string for execution
  (e.g., `-c`).
- **Source Cmd** — The command used to source environment files (e.g.,
  `source` for bash, `.` for POSIX sh).

### Saving

Click **Save Shells** to write the shells file to disk.

---

## GREX Settings

Click the **Grex Config** button in the header bar to open GREX's own settings
dialog.

Currently, the only setting is:

- **File Editor** — The command GREX uses to open files for external editing.
  The default is `xterm -e vim`. Change this to your preferred terminal editor
  command (e.g., `alacritty -e nano`, `gnome-terminal -- vim`,
  `xdg-open`).

GREX stores its settings at `~/.config/grex/grex.ini` (or under
`$XDG_CONFIG_HOME/grex/` if that variable is set). The format is simple
key-value:

```ini
file_editor=xterm -e vim
```

---

## Rex Concepts Quick Reference

| Concept | Meaning |
|---|---|
| **Project** | A Rex workspace anchored by a `rex.config` file. All paths are resolved relative to the project root. |
| **Config** | The `rex.config` file. Defines the project root, where to find units and shells, and any custom key-value settings. |
| **Plan** | An ordered list of tasks for Rex to execute sequentially. |
| **Task** | A single entry in a plan. Each task references a unit by name and optionally carries a comment. |
| **Unit** | A definition of something Rex can execute — a script, command, or binary — along with its execution context and error handling behavior. |
| **Shell** | An interpreter definition (name, binary path, execution argument, source command) that Rex uses to run shell-command units. |
| **Variable** | A `${VAR}` pattern in a config value, resolved from environment variables or manual overrides. Enables portable configs across machines. |
| **Rectification** | Rex's self-healing mechanism. When a unit's target fails and rectification is enabled, Rex runs the rectifier script. Used for verification-and-fix or preventive recovery patterns. |

---

## License

GREX is licensed under the
[GNU Affero General Public License v3.0](https://www.gnu.org/licenses/agpl-3.0.html)
(AGPL-3.0).

Copyright (C) 2025, SILO GROUP LLC. Written by Chris Punches.

---

## Links

- [Rex](https://git.silogroup.org/SILO-GROUP/rex) — The Rex execution engine
- [SILO GROUP](https://www.silogroup.org) — Distributed systems and consulting
- [Dark Horse Linux](https://www.darkhorselinux.org) — The Linux distribution Rex was originally built for
