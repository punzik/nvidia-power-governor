# AGENTS.md

Guidance for coding agents working in this repository.

## General approach

- Prefer KISS (Keep It Simple): choose straightforward, easy-to-review designs over clever or over-generalized solutions.
- If there is uncertainty about protocol behavior, architecture, naming, or trade-offs, ask the user before implementing instead of guessing.
- Avoid touching unrelated source files when making a targeted fix.
- Always confirm with the user before executing significant changes. This includes all file edits and Git operations (such as commits and pushes). Do not proceed without explicit approval.
