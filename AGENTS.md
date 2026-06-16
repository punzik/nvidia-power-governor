# AGENTS.md

Guidance for coding agents working in this repository.

## General approach

- Prefer KISS (Keep It Simple): choose straightforward, easy-to-review designs over clever or over-generalized solutions.
- If there is uncertainty about protocol behavior, architecture, naming, or trade-offs, ask the user before implementing instead of guessing.
- Avoid touching unrelated source files when making a targeted fix.
- Always confirm with the user before executing significant changes. This includes all file edits. Do not proceed without explicit approval.

## Git commits — strict rules

- **Never** run Git commands (commit, push, merge, rebase, etc.) without explicit user confirmation.
- Before committing, ask the user (e.g. "Commit?") and **stop**. Do not include any `git commit` command in the same message.
- **Only** an explicit "Yes" or "Да" in a **separate user message** means approval.
- If the user gives a different task, answer, or anything other than a clear "Yes"/"Да", treat it as **not approved**. Complete the other task first, then ask again.
- Never assume that the user's response to a different question implies approval for a pending confirmation.
- If you accidentally commit without approval, immediately inform the user and update this file to prevent recurrence.
