# Pultsari

- The browser game always runs on port 3000 (`npm start`, http://localhost:3000). The port is fixed in `web/server.js`; do not start servers on other ports. If 3000 is taken, stop the old game server first, then start the new one.

## Who does the work: local LLM first, Claude orchestrates (Tero, 17.11.2026)

In this project the Claude main session (Opus 5.5) **only orchestrates**. The actual work (coding,
tests, art, text) is done by the local LLM on Tero's Windows PC: Qwen3.8-35B-A3B, see the
`local-llm` skill. Claude may do a task itself, or hand it to any other model it needs, only when
the task is too hard for the local model.

This replaces the global "workflows by default" rule here: do not spawn Claude subagents or
Workflow agents for routine implementation. They are for escalation only.

**Orchestrator (Claude) does:** talk with Tero, plan, split work into small self-contained tasks,
write the briefs, review every diff, run builds and tests, browser checks, commit.

**Worker (local LLM):**
- Code changes / anything needing repo tools: `claude-local -p "<brief>" --permission-mode acceptEdits`
  with cwd = the checkout being worked on. Needs the sandbox bypassed (Tailscale). Takes ~40 s
  minimum; run long tasks with `run_in_background`.
- Text-only tasks (summaries, commit-message drafts, Finnish game texts, data): `ai-route local "..."`.
- At most 2 workers at once (the server has 2 slots), and only on disjoint files.
- If the PC is unreachable (off, Tero gaming), tell Tero and ask: start it, or let Claude do the task.
  Never run `qwen --stop` unless Tero asks.

**Brief:** the worker sees nothing of the conversation. Give the files to touch, what to change,
acceptance criteria and the command that proves it (`make test`, `make`). Tell it: no commits,
no servers (port 3000 rule), no edits outside the named files.

**Check every result:** read `git diff`, run `make test` (and the browser check when the change
is visible). Failed or wrong → one retry with a sharper brief that quotes the error. Second
failure, or a task that is clearly hard from the start (root cause of a tricky bug, architecture,
cross-cutting refactor, subtle game-logic or timing math) → escalate: Claude does it itself or
hands it to a Claude subagent / workflow / codex per the `model-tiers` skill. Say in one line
why you escalated (`→ opus: <reason>`), and announce delegations the same way (`→ local: <task>`).
