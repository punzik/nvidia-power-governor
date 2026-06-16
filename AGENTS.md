# AGENTS.md

Guidance for coding agents working in this repository.

## General approach

- Prefer KISS (Keep It Simple): choose straightforward, easy-to-review designs over clever or over-generalized solutions.
- If there is uncertainty about protocol behavior, architecture, naming, or trade-offs, ask the user before implementing instead of guessing.
- Avoid touching unrelated source files when making a targeted fix.
- When the user asks for a code change, file edits needed for that task are allowed.
- Ask for confirmation before broad, destructive, unrelated, irreversible, or ambiguous changes.
- Do not treat a request to inspect, explain, or plan as permission to edit files.

## Git commands — strict rules

- Never run repository-changing Git commands without explicit user approval.
- Repository-changing Git commands include, but are not limited to:
  - `git commit`
  - `git push`
  - `git merge`
  - `git rebase`
  - `git reset`
  - `git checkout`
  - `git switch`
  - `git branch -D`
  - `git tag`
  - any command that changes repository history, branches, tags, remotes, the index, or the working tree
- Read-only Git commands are allowed when useful, for example:
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

Result: do not commit and do not push. The message contains extra words, so it is not exact approval.

## If a commit happens without approval

If the agent accidentally commits without exact approval:

1. Immediately inform the user.
2. Show the commit hash.
3. Do not push.
4. Ask the user whether to keep or revert the commit.
5. Update this file or propose an update to prevent recurrence.
