@echo off
REM ##############################################################################
REM Copyright (c) 2025
REM
REM Author(s):
REM  The Jamulus Development Team
REM
REM ##############################################################################
REM
REM This program is free software; you can redistribute it and/or modify it under
REM the terms of the GNU General Public License as published by the Free Software
REM Foundation; either version 2 of the License, or (at your option) any later
REM version.
REM
REM This program is distributed in the hope that it will be useful, but WITHOUT
REM ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
REM FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
REM details.
REM
REM You should have received a copy of the GNU General Public License along with
REM this program; if not, write to the Free Software Foundation, Inc.,
REM 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
REM
REM ##############################################################################

echo Setting up Git configuration for Jamulus repository...
echo.

REM Configure the "ours" merge driver
REM This is used in .gitattributes to keep local versions of certain files
REM during merge operations (e.g., customized workflow configurations)
git config --local merge.ours.driver "true"
git config --local merge.ours.name "Keep local version during merge"

if %errorlevel% neq 0 (
    echo Error: Failed to configure Git. Make sure you're in a Git repository.
    exit /b 1
)

echo Git configuration complete!
echo.
echo The following merge driver has been configured:
echo   merge.ours.driver = true
echo.
echo This allows .gitattributes to preserve your local versions of files
echo marked with 'merge=ours' when merging changes from other branches.
