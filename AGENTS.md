# AGENTS.md

Guidance for coding agents working in this repository.

## General approach

- Prefer KISS (Keep It Simple): choose straightforward, easy-to-review designs over clever or over-generalized solutions.
- If there is uncertainty about protocol behavior, architecture, naming, or trade-offs, ask the user before implementing instead of guessing.
- Avoid touching unrelated source files when making a targeted fix.
- Always confirm with the user before executing significant changes. This includes all file edits. Do not proceed without explicit approval.
- **Never** run Git commands (commit, push, merge, rebase, etc.) without explicit user confirmation. Always ask and wait for a clear "Yes" before running `git commit`. Do not commit on your own initiative under any circumstances.
- When you ask for confirmation (e.g. "Commit?" or "Ready for the next step?"), **only** an explicit "Yes" means approval. If the user gives a different task, answer, or anything other than a clear "Yes", treat it as **not approved** — complete the other task first and wait for explicit approval before proceeding with the pending action.
- Never assume that the user's response to a different question implies approval for a pending confirmation.
