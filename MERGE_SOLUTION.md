# Solution: Preventing Merge Conflicts on autobuild.yml

## Manual Conflict Resolution (Simplest Method)

When merging from master into my-build and you get a conflict on `autobuild.yml`, choose:

**→ "Accept Current Change"** (or "Keep Ours" in some tools)

This keeps your my-build version and discards the incoming master version.

## Automated Solution (For Future Merges)

To avoid manual resolution in the future:

1. ✅ `.gitattributes` file with `merge=ours` for autobuild.yml
2. ✅ Git config: `merge.ours.driver = true`

Once this PR is merged into my-build, the `.gitattributes` file will be in place and future merges will automatically keep your version without conflicts.
