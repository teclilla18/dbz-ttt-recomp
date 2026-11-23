# DBZ Tenkaichi Tag Team recompilation research

Work toward a native port of the PSP game for Nintendo Switch and Switch 2.
PC runs are for development and checks. There is no playable port or adopted
recompiler/runtime yet. Phase 0 is complete; the first Phase 1
[tool evaluation](docs/notes/tool-evaluation.md) found useful components and gaps.
Goal order: stable PC checks, Switch, Switch 2, then online.

Read [AGENTS.md](AGENTS.md), [STATE.md](STATE.md), the
[recon](docs/notes/recon.md) and [plan](docs/notes/plan.md).

This repository must stay private until the owner decides otherwise. It contains
only original project text, repository safeguards and synthetic tool probes. No game files, generated
game code, saves, firmware or PSP decryption keys may be committed or uploaded.
Each player will need their own game copy and local setup process.

## Repository safeguards

After cloning, install the tracked hooks locally:

```sh
git config --local core.hooksPath tools/git-hooks
```

Python 3 and Git are required. Before every commit and push:

```sh
python tools/check_no_game_data.py --staged
git status --short --ignored
```

After the first commit, also run:

```sh
python tools/check_no_game_data.py --history
```

The pre-commit hook checks every staged blob. The pre-push hook also checks all
local reachable history, including deleted files. Do not bypass either hook.
The checks reject game/build paths, binary formats, known game hashes, large
files, likely disassembly, generated game code, keys and personal paths/emails.
They are a safety net: review the staged file list and contents too. Git hooks
must be installed in each new clone; they are not a server-side guarantee.

Keep the game in ignored `baserom/`, generated output in ignored `build/`, and
machine-specific notes in ignored `AI_HANDOFF.md` or `.local/`. External research
tools stay outside this repository. No third-party source has been adopted and
the project licence has not been chosen.

## Research credits

Thanks to these projects and authors for public tools, documentation and ideas.
This is research credit, not a claim that their code is included or that they
endorse this project. [Research notes](docs/notes/research.md) record what we
learned, licence boundaries, tested facts and limits for each source.

- Static recompilation: [N64Recomp](https://github.com/N64Recomp/N64Recomp),
  [Zelda64Recomp](https://github.com/Zelda64Recomp/Zelda64Recomp),
  [N64ModernRuntime](https://github.com/N64Recomp/N64ModernRuntime),
  [XenonRecomp](https://github.com/hedge-dev/XenonRecomp),
  [UnleashedRecomp](https://github.com/hedge-dev/UnleashedRecomp),
  [XenosRecomp](https://github.com/hedge-dev/XenosRecomp) and
  [PS2Recomp](https://github.com/ran-j/PS2Recomp).
- PSP toolkits: [sp00nznet](https://github.com/sp00nznet/psprecomp),
  [jessicanataliagta](https://github.com/jessicanataliagta/PSPRecomp),
  [Tzeentchnet](https://github.com/Tzeentchnet/psp-recomp) and
  [wizardengineer](https://github.com/wizardengineer/psprecomp).
- PSP behaviour and interfaces: [PPSSPP](https://github.com/hrydgard/ppsspp),
  [JPCSP](https://github.com/jpcsp/jpcsp) and
  [pspdev / PSPSDK](https://github.com/pspdev/pspsdk).
- Inspection tools: [John-K/pspdecrypt](https://github.com/John-K/pspdecrypt),
  [kotcrab/ghidra-allegrex](https://github.com/kotcrab/ghidra-allegrex) and
  [pspdev/prxtool](https://github.com/pspdev/prxtool).
- This game's research: [MarquitoK/TTTRecomp](https://github.com/MarquitoK/TTTRecomp),
  [kastomd/Editor_indice_Py](https://github.com/kastomd/Editor_indice_Py) and
  [Aluigi's archive format reference](https://aluigi.altervista.org/bms/tenkaichi_pack.bms).
  [Nimbus](https://github.com/thebabyhippo303/nimbus-recompilation-project)
  was also surveyed; its closed source was not selected for reuse.
- Porting method: [Sonic Riders](https://github.com/teclilla18/sonic-riders-recomp),
  [Sonic Heroes](https://github.com/teclilla18/sonic-heroes-recomp),
  [SwitchWakerHD](https://github.com/centollOS/SwitchWakerHD),
  [Shipwright-Switch](https://github.com/timschneeb/Shipwright-Switch) and
  [Harbour Masters' Shipwright](https://github.com/HarbourMasters/Shipwright).
- Switch homebrew: [libnx](https://github.com/switchbrew/libnx),
  [deko3d](https://github.com/devkitPro/deko3d) and [uam](https://github.com/devkitPro/uam).
