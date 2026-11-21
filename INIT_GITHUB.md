# INIT_GITHUB — Repository setup and git workflow

Upstream: https://github.com/guilhermepachecod/activities-overview
Main branch: `main`

This file is meant for humans **and** coding agents working on this project. Read `PROJECT.md` first.

---

## 0. Golden rule: never break the host's configuration

This project is developed on other people's machines and servers. Agents and scripts **must not change any existing configuration**. Always validate first, and act only on what is missing.

- **Never** run `git config --global` or `git config --system`.
- **Never** overwrite an existing value (`user.name`, `user.email`, remotes, credential helpers, SSH keys, `~/.gitconfig`, `~/.ssh/`).
- **Never** generate SSH keys, tokens or credential helpers on your own.
- If something required is missing or failing, **stop and ask the user**. Do not improvise workarounds.

---

## 1. Initial setup (once)

### 1.1 Check the existing state
```bash
git rev-parse --is-inside-work-tree 2>/dev/null && echo "repo: exists" || echo "repo: missing"
git config user.name  || echo "user.name: missing"
git config user.email || echo "user.email: missing"
git remote get-url origin 2>/dev/null || echo "origin: missing"
```

### 1.2 Act only on what is missing
| Item | Missing → action | Exists → action |
|---|---|---|
| Repo | `git init && git branch -M main` | nothing |
| `user.name` / `user.email` | **ask the user** for the values, then set them **locally only**: `git config user.name "..."` / `git config user.email "..."` | nothing |
| `origin` | `git remote add origin https://github.com/guilhermepachecod/activities-overview.git` (or the user's fork URL) | **keep it as is**, whether it is HTTPS or SSH |

### 1.3 `.gitignore` (create it if missing; if it exists, only append the lines that are missing)
```gitignore
build/
builddir/
*.o
*.so
*.a
compile_commands.json
*.deb
*.rpm
debian/.debhelper/
debian/activities-overview/
debian/files
debian/*.substvars
debian/*.log
.vscode/
.idea/
*.swp
*~
core
vgcore.*
```

### 1.4 First commit and push
```bash
git add .
git commit -m "chore: initial commit"
git push -u origin main
```

### 1.5 If the push fails
**Stop and report the error to the user.** Common causes, for the user to decide on:
- `Permission denied (publickey)`: `origin` uses SSH and the machine has no key registered on GitHub. The user can switch to HTTPS with `git remote set-url origin https://github.com/<owner>/activities-overview.git`, or register a key.
- `Authentication failed` over HTTPS: there is no credential or token configured.
- `rejected (fetch first)`: the remote has commits you don't have. Run `git pull --rebase origin main` and push again.

---

## 2. Standard workflow: every change ends with a push

```bash
git pull --rebase origin main          # sync before starting

# ... changes ...

meson compile -C build                 # once meson.build exists
meson test -C build

git status
git add <files>
git commit -m "feat(grid): animate windows from real geometry to grid"
git push origin main
```

### Rules
- No work stays local only: every finished logical unit gets a commit and a push.
- Do not push a broken build.
- Keep commits small, one topic per commit.
- Never commit `build/`, binaries, `.deb`/`.rpm` files, credentials or tokens.
- Never `git push --force` to `main`. If the push is rejected: `git pull --rebase origin main`, resolve the conflicts, then push again.
- When a roadmap milestone is completed, say so in the commit: `feat: complete M2 window grid`.
- CI status can be checked on the repository's **Actions** tab, or with `gh run list --limit 1` if `gh` is already authenticated. Do not try to authenticate `gh` yourself.

### Commit prefixes (Conventional Commits, in English)
| Prefix | Use |
|---|---|
| `feat:` | New feature |
| `fix:` | Bug fix |
| `perf:` | Performance |
| `refactor:` | Refactoring with no behavior change |
| `build:` | meson, dependencies, packaging |
| `ci:` | GitHub Actions |
| `docs:` | README, PROJECT.md |
| `test:` | Tests |
| `chore:` | General maintenance |

Optional scope: `grid`, `search`, `workspaces`, `superkey`, `thumbnail`, `layout`, `anim`, `packaging`.

---

## 3. Releases (from M8 on, maintainer only)

```bash
# update the version in meson.build
git commit -am "build: bump version to 0.1.0"
git tag -a v0.1.0 -m "v0.1.0"
git push origin main --tags
```
A `v*` tag triggers `release.yml`, which builds the `.deb` and `.rpm` packages and attaches them to the GitHub Release.
Versioning follows SemVer: `0.x` until the MVP is done, `1.0.0` once the acceptance criteria in `PROJECT.md` are met.

---

## 4. External contributors

Fork the repo, work in a branch and open a PR against `main`:
```bash
git switch -c feat/workspace-dnd
git push -u origin feat/workspace-dnd
```
Then open the PR on GitHub (or with `gh pr create --fill --base main` if `gh` is already authenticated).