# Solution: Preventing Merge Conflicts on autobuild.yml

## The Problem

You're trying to merge changes from `master` into your `my-build` branch and getting conflicts on `autobuild.yml`.

## Solution: Manual Conflict Resolution on GitHub

When merging a PR from `master` to `my-build` on GitHub:

1. Click "Resolve conflicts" button
2. In the conflict editor, you'll see:
   ```
   <<<<<<< my-build
   [Your custom autobuild.yml content]
   =======
   [Content from master]
   >>>>>>> master
   ```
3. **Keep ONLY your custom content:**
   - Delete the line: `<<<<<<< my-build`
   - Keep your custom autobuild.yml content (everything between `<<<<<<<` and `=======`)
   - Delete the line: `=======`
   - Delete all content from master (everything between `=======` and `>>>>>>>`)
   - Delete the line: `>>>>>>> master`
4. Click "Mark as resolved"
5. Click "Commit merge"

**Result:** Your my-build version of `autobuild.yml` is kept, master's version is discarded.

The merge will create a new commit on your `my-build` branch. This is correct.

## Manual Conflict Resolution - Command Line

If you're using git locally on your `my-build` branch:

```bash
git checkout my-build
git merge master
```

When you get a conflict on `autobuild.yml`, choose **"Accept Current Change"** (or "Keep Ours" in some tools):

- **"Current"** = your my-build branch (what you want to KEEP)
- **"Incoming"** = changes from master (what you want to DISCARD)

## Automated Solution (For Future Merges)

To avoid manual resolution in the future:

1. ✅ `.gitattributes` file with `merge=ours` for autobuild.yml
2. ✅ Git config: `merge.ours.driver = true`

Once this PR is merged into my-build, the `.gitattributes` file will be in place and future merges will automatically keep your version without conflicts.
