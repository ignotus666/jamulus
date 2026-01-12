# Solution: Preventing Merge Conflicts on autobuild.yml

Your `.gitattributes` file and Git configuration are correct. The setup will work for future merges.

## What You Have

1. ✅ `.gitattributes` file with `merge=ours` for autobuild.yml
2. ✅ Git config: `merge.ours.driver = true`

## Why You're Still Getting Conflicts

The `.gitattributes` file needs to exist in **your my-build branch** for the merge strategy to work.

## Simple Fix

Merge this PR into your my-build branch. After that, all future merges from master → my-build will automatically keep your version of autobuild.yml without conflicts.

That's it! No scripts, no complex setup needed.
