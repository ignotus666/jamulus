# Solution: Preventing Merge Conflicts on autobuild.yml

## IMPORTANT: Change PR Base Branch First!

**The PR must target `my-build`, not `master`!**

If you see "Resolving conflicts between master and my-build and committing changes → master", the PR is targeting the wrong branch.

**To fix this:**
1. Go to your PR page on GitHub
2. Click "Edit" next to the title (or look for base branch settings)
3. Change the base branch from `master` to `my-build`
4. The PR should now show: `my-build ← copilot/update-gitattributes-for-autobuild`

Once the base branch is correct, resolve any conflicts using the instructions below.

## Manual Conflict Resolution - GitHub Web Interface

If you're resolving conflicts **on the GitHub website** when merging this PR into `my-build`:

1. Click "Resolve conflicts" button on the PR
2. In the conflict editor, you'll see markers like:
   ```
   <<<<<<< my-build
   [Your custom autobuild.yml content]
   =======
   [Content from this PR/master]
   >>>>>>> copilot/update-gitattributes-for-autobuild
   ```
3. **Delete everything EXCEPT your custom content**:
   - Delete the `<<<<<<< my-build` line
   - Keep your custom autobuild.yml content (the section between `<<<<<<<` and `=======`)
   - Delete the `=======` line
   - Delete all content from the incoming branch (between `=======` and `>>>>>>>`)
   - Delete the `>>>>>>> copilot/update-gitattributes-for-autobuild` line
4. Click "Mark as resolved"
5. Click "Commit merge"

**Result:** Your custom `autobuild.yml` is preserved in the `my-build` branch.

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
