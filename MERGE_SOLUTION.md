# Solution: Preventing Merge Conflicts on autobuild.yml

## Manual Conflict Resolution (Simplest Method)

**Important:** Make sure you're ON your `my-build` branch when merging:
```bash
git checkout my-build
git merge master
```

When you get a conflict on `autobuild.yml`, choose:

**→ "Accept Current Change"** (or "Keep Ours" in some tools)

- **"Current"** = your my-build branch (what you want to KEEP)
- **"Incoming"** = changes from master (what you want to DISCARD)

The changes will be committed to your `my-build` branch, NOT to master. This is correct and what you want.

## Automated Solution (For Future Merges)

To avoid manual resolution in the future:

1. ✅ `.gitattributes` file with `merge=ours` for autobuild.yml
2. ✅ Git config: `merge.ours.driver = true`

Once this PR is merged into my-build, the `.gitattributes` file will be in place and future merges will automatically keep your version without conflicts.
