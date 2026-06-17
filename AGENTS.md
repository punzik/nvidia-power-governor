# AGENTS.md

Guidance for coding agents working in this repository.

## General approach

- Prefer KISS (Keep It Simple): choose straightforward, easy-to-review designs over clever or over-generalized solutions.
- If there is uncertainty about protocol behavior, architecture, naming, or trade-offs, ask the user before implementing instead of guessing.
- Avoid touching unrelated source files when making a targeted fix.
- When the user asks for a code change, file edits needed for that task are allowed.
- Ask for confirmation before broad, destructive, unrelated, irreversible, or ambiguous changes.
- Do not treat a request to inspect, explain, or plan as permission to edit files.

## Plan creation rules

When the user asks to create or update a plan (saved as `*.md` in the repository root), the plan must be self-contained: a model reading it without any other context must be able to implement the changes without asking questions.

### Structure

A plan has two parts:

1. **Task description** — what is being changed and why.
2. **Implementation steps** — ordered list of file-level changes.

### Task description must include

- **Current state** — how the code works now (relevant structures, algorithm, data flow).
- **New algorithm / behavior** — exact logic with pseudocode or formulas. No vague descriptions like "improve X" — show the actual conditionals, thresholds, and edge cases.
- **Examples of behavior** — at least one worked-through scenario showing input → output for each code path (normal, boundary, priority conflict).
- **Structural changes** — exact new/removed/renamed fields in every affected struct, with types and comments.
- **Differences table** — "was / becomes" comparison for key parameters.

### Implementation steps must include

- Each step targets one or more related files.
- For each file: list the exact changes (additions, removals, renames) with enough detail to apply without guessing.
- Include the new function signatures with full parameter lists and return types.
- Include pseudocode or code snippets for non-trivial logic.
- If test files change: list which existing tests to remove, which new tests to add (with scenario description), and which existing tests to update.
- Steps must be ordered by dependency: header files before implementation files, implementation before tests.

### What to avoid

- Do not write "update config parser" without listing the new keys and their validation rules.
- Do not write "refactor main loop" without showing the before/after code blocks.
- Do not omit edge cases (clamping, error paths, priority conflicts).
- Do not leave fields unnamed or untyped — every struct field must have name, type, and description.
- Do not mix steps for unrelated files in one step — group only files that change together (e.g., header + its implementation).

## Git commands — strict rules

Git commands are divided into three categories: read-only, local-changing, and remote-changing.

### Remote-changing Git commands: absolutely forbidden

The agent must never run remote-changing Git commands under any circumstances.

Forbidden commands include, but are not limited to:

- `git push`
- `git push --force`
- `git push --force-with-lease`
- `git push origin ...`
- `git push --tags`
- `git push --delete ...`
- any alias, script, shell command, IDE action, task runner, or tool call that performs a Git push
- any command that uploads commits, branches, tags, refs, or repository state to a remote

User approval does not override this rule.

Even if the user says `push`, `yes push`, `да, push`, `commit and push`, or gives any other explicit push instruction, the agent must refuse to push and must explain that pushing is outside the allowed actions.

The agent may prepare local changes or a local commit when allowed by the commit protocol below, but the user must perform any push manually.

### Local-changing Git commands: approval required

Never run local repository-changing Git commands without explicit user approval.

Local repository-changing Git commands include, but are not limited to:

- `git commit`
- `git merge`
- `git rebase`
- `git reset`
- `git checkout`
- `git switch`
- `git branch -D`
- `git tag`
- any command that changes repository history, branches, tags, the index, or the working tree

### Read-only Git commands: allowed

Read-only Git commands are allowed when useful, for example:

- `git status`
- `git diff`
- `git log`
- `git show`

## Git commits — hard approval protocol

Git commit approval is a state machine.

### State: no commit approval

This is the default state.

In this state:

- Do not commit.
- Do not prepare a commit unless the user explicitly asks for it.
- If the work is ready and a commit may be useful, ask the user: `Commit these changes?`
- After asking, stop. Do not include `git commit` or any equivalent command in the same assistant turn.

### State: commit pending

The agent enters this state only after asking the user whether to commit.

While commit approval is pending:

- The next user message is approval only if the entire message is exactly one of:
  - `Yes`
  - `yes`
  - `Да`
  - `да`
- Any other message is not approval.
- If the message contains a new task, instruction, explanation, question, filename, command, bug report, condition, or any extra words, it is not approval.
- Messages such as `fix X`, `also change Y`, `first do Z`, `yes, but fix X`, `да, но сначала...`, or `ок, поправь README` are not approval.
- Do not infer approval from context, tone, continuation, urgency, or intent.

If the next user message is not exact approval:

1. Clear the commit-pending state.
2. Do not commit.
3. Complete the new user task if it is safe and allowed.
4. After completing that task, ask again: `Commit these changes?`
5. Stop.

### Commit execution rule

A commit is allowed only when all of the following are true:

1. The assistant previously asked whether to commit.
2. The immediately preceding user message consists only of `Yes`, `yes`, `Да`, or `да`.
3. No other task, instruction, or message appeared between the commit question and the approval.
4. The approval applies only to the current assistant turn.

Approval expires after one assistant turn.

Approval cannot be carried over across tasks.

Approval cannot be inferred from a response to a different question.

Approval cannot be inferred from a user message that contains both approval and another task.

## Commit examples

Example 1:

Assistant: `Commit these changes?`

User: `Да`

Result: commit is allowed for this assistant turn only.

Example 2:

Assistant: `Commit these changes?`

User: `Fix the tests first.`

Result: do not commit. Fix the tests, then ask again.

Example 3:

Assistant: `Commit these changes?`

User: `Да, но сначала обнови changelog.`

Result: do not commit. Update the changelog, then ask again.

Example 4:

Assistant: `Commit these changes?`

User: `yes, also push it`

Result: do not commit and do not push. The message contains extra words, so it is not exact commit approval. Push is forbidden in all cases.

Example 5:

User: `Commit and push the changes.`

Result: pushing is forbidden. The agent may ask whether to create a local commit, but must not push.

## If a commit or push happens without approval

If the agent accidentally commits without exact approval:

1. Immediately inform the user.
2. Show the commit hash.
3. Do not push.
4. Ask the user whether to keep or revert the commit.
5. Update this file or propose an update to prevent recurrence.

If the agent accidentally pushes:

1. Immediately inform the user.
2. Show the exact command that was run if known.
3. Show the pushed branch, remote, and commit hash if known.
4. Do not attempt to fix the remote state automatically.
5. Ask the user what recovery action they want to perform manually.
6. Treat the incident as a critical protocol failure.
