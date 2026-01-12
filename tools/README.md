# Jamulus Tools Directory

This directory contains various utility scripts for Jamulus development and maintenance.

## Git Configuration Setup

### setup-git-config.sh (Linux/macOS) and setup-git-config.bat (Windows)

These scripts configure the Git merge driver required for the `.gitattributes` file to work correctly.

**Purpose:** The repository uses a custom merge strategy for certain files (like `.github/workflows/autobuild.yml`) to preserve local customizations when merging changes from other branches. This requires configuring a local Git merge driver.

**Usage:**

On Linux/macOS:
```bash
./tools/setup-git-config.sh
```

On Windows:
```cmd
tools\setup-git-config.bat
```

**When to run:** Run this script once after cloning the repository if you have customized workflow files or other files marked with `merge=ours` in `.gitattributes` that you want to preserve during merges.

**What it does:** Configures the `merge.ours.driver` in your local Git configuration to keep your local version of files when conflicts occur during merge operations.

## Other Tools

- **changelog-helper.sh** - Helper script for generating changelog entries
- **check-wininstaller-translations.sh** - Validates Windows installer translations
- **checkkeys.pl** - Checks keyboard shortcuts in translation files
- **create-translation-issues.sh** - Creates GitHub issues for translation updates
- **generate_json_rpc_docs.py** - Generates documentation for JSON-RPC API
- **get_release_contributors.py** - Generates contributor list for releases
- **qt5_to_qt6_country_code_table.py** - Converts Qt5 country codes to Qt6
- **update-copyright-notices.sh** - Updates copyright year in source files
