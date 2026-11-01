# COS214-Project-2026
## Branch Workflow

**`main`** is locked. Do NOT push or merge into it. It only receives the final, finished project from `dev`.

**`dev`** is the integration branch. All work is merged here first.

### Getting the `dev` branch

New clone:
```bash
git clone https://github.com/MogaleLebethe/COS214-Practical-6.git
cd COS214-Practical-6
git checkout dev
```

Already cloned:
```bash
git fetch origin
git checkout dev
git pull
```

You can also pick `dev` from the branch dropdown on GitHub.

### Adding your work

1. Start from the latest `dev`:
```bash
   git checkout dev
   git pull
   git checkout -b your-branch-name
```
2. Work, then commit and push:
```bash
   git add .
   git commit -m "Describe what you changed"
   git push -u origin your-branch-name
```
3. On GitHub, open a Pull Request with **base: `dev`** (not `main`).
4. After review, merge it into `dev`.

### Final release
When the project is complete and tested, one Pull Request merges `dev` into `main`.
=======
# COS214-Project-2026
>>>>>>> 8efbc07 (Rename project from Practical-6 to Project-2026)
