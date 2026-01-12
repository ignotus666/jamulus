#!/bin/bash
##############################################################################
# Copyright (c) 2025
#
# Author(s):
#  The Jamulus Development Team
#
##############################################################################
#
# This program is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation; either version 2 of the License, or (at your option) any later
# version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
# details.
#
# You should have received a copy of the GNU General Public License along with
# this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
#
##############################################################################

set -eu -o pipefail

echo "Setting up Git configuration for Jamulus repository..."

# Configure the "ours" merge driver
# This is used in .gitattributes to keep local versions of certain files
# during merge operations (e.g., customized workflow configurations)
git config --local merge.ours.driver "true"
git config --local merge.ours.name "Keep local version during merge"

echo "✓ Git configuration complete!"
echo ""
echo "The following merge driver has been configured:"
echo "  merge.ours.driver = true"
echo ""
echo "This allows .gitattributes to preserve your local versions of files"
echo "marked with 'merge=ours' when merging changes from other branches."
