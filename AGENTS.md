# Instructions for coding agents (Codex, Claude Code and others)

Goal: play Dragon Ball Z: Tenkaichi Tag Team (PSP) natively on a modded Nintendo Switch and on Switch 2 by static
recompilation: the game's MIPS (Allegrex) code becomes C on each player's machine from their own copy of the game;
this repo holds only our source. Order of goals: 1) boots and runs stable on PC, 2) Switch, 3) Switch 2,
4) online play (the game's ad-hoc multiplayer over the internet). Online only after it runs stable.

Same way of working as the user's Sonic Riders and Sonic Heroes ports (read them, don't copy blindly):
- `<home>/Desktop/Sonic Riders Decompile` (CLAUDE.md, AI_HANDOFF.md, docs/notes/, tools/recomp/, runtime/)
- `<home>/Desktop/Sonic Heroes Decompile` (how a second game was started: docs/notes/recon.md, docs/HISTORY.md)
Those are GameCube (PowerPC) ports. A PSP is different hardware: borrow the method, the project layout, the
rules, the Switch platform layer and the online lockstep ideas, not the CPU or graphics code.

## Hard rules

1. **Game-data guard.** Never commit, paste or upload anything from the game: ISO/CSO images, EBOOT.BIN, PRX
   modules, decrypted executables, extracted assets, disassembly, recompiled output (`build/`), saves, or any
   PSP decryption keys. Small facts (addresses, function names, NIDs) are fine; blocks of code are not. Add a guard
   script (like Riders' tools/check_no_game_data.py) and a pre-commit hook before the first real commit.
2. The GitHub repo is **public** as a work in progress (user's decision, 2026-11-23). Everything pushed is world-readable:
   rules 1 and 4 apply to every commit and its message. Push only when the user asks.
3. Never modify the user's original game file. Work on a copy in `baserom/` (gitignored).
4. No personal details in tracked files (home paths, drive letters, emails): write `<home>`, `<repo>`, `<sd>`.
5. **Learn from others first (the user's wish, repeated 2026-11-23).** Every task, for Claude Code and Codex alike:
   search online first and skip as much work as possible where others already have what we need. Check the
   user's own repos (https://github.com/teclilla18, e.g. sonic-riders-recomp's renderer and Switch layer) and
   similar projects, then build only the missing part. Before building any part, search online for projects that already
   solved it (static recompilers, PSP emulators, decompilation tools, Switch homebrew) and reuse what works: their
   tools as they are, their code where the licence allows, their ideas always. Record every source in
   `docs/notes/research.md` (link, licence, what we took or learned) and credit it in the README.
   Licences: MIT/BSD/zlib code can be copied with credit. GPL code (PPSSPP is GPL-2.0+, the best reference for PSP
   behaviour) makes this project GPL if copied in: ask the user once whether the project may be GPL; until then
   read it and learn from it, don't paste it. Never take leaked or proprietary SDK code or keys.
6. Plan first, show the user, then build. One task at a time; finish, test, commit, then the next.

## Status words (same as Riders)

Keep apart: **implemented** (code in), **tested** (by us, say how), **accepted** (the user played it and said it
is fine), **released**. Evidence tags: [confirmed] / [likely] / [uncertain]. A partial test is not validation.

## Files to keep (same as Riders)

- `STATE.md`: current status, rewritten (not appended) at the end of each session, under ~120 lines.
- `docs/HISTORY.md`: a short dated entry per session.
- `docs/notes/<topic>.md`: durable facts (addresses, formats, procedures). `docs/notes/plan.md` is the plan below.
- `AI_HANDOFF.md` (local, gitignored): Claude and Codex both use it; claim an item before editing its files.

## Plain words

The user is not a programmer by trade and likes short, clear, numbered steps. Plain language in notes and replies.
