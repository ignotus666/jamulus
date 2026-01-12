# Personal Modifications to GitHub Actions

## Problem
This fork contains personal modifications to the GitHub Actions workflow file (`.github/workflows/autobuild.yml`) that are customized for personal use. When upstream PRs try to merge changes to this file, it creates merge conflicts that prevent the PR from being merged.

## Solution
The repository uses a `.gitattributes` file with a custom merge strategy to preserve local modifications to the autobuild workflow file while allowing other files to merge normally.

### How it works
1. The `.gitattributes` file marks `.github/workflows/autobuild.yml` with `merge=ours` strategy
2. Git's "ours" merge driver (configured via `git config merge.ours.driver true`) keeps the local version during conflicts
3. When merging PRs, changes to other files merge normally, but autobuild.yml remains unchanged

### Setup for new clones
After cloning this repository, run:
```bash
git config merge.ours.driver true
```

This configures the merge driver that the `.gitattributes` file references.

### What this means
- **Incoming PRs**: Changes to `.github/workflows/autobuild.yml` from upstream will be automatically ignored during merge
- **Other files**: All other files will merge normally
- **Your modifications**: Your personal workflow modifications are preserved

### Important notes
- This only affects merges on your local repository/fork
- If you want to adopt upstream changes to the workflow file, you'll need to manually apply them
- The `.gitattributes` file must be committed to the repository for this to work
- Each collaborator needs to run the git config command after cloning

## Alternative approaches considered
1. **Keeping workflow in a separate branch**: Would require constant rebasing and cherry-picking
2. **Using git merge -X ours**: Would require manual intervention for each merge
3. **Forking the workflow file with a different name**: Would break GitHub Actions functionality
4. **Using .gitattributes with merge=ours**: ✓ Selected - Most automated and maintainable solution
