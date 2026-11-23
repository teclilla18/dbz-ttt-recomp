# History

## 2026-11-22: Phase 0 recon and repository safeguards

- Read project instructions/plan first, then reread the user's expanded research
  instructions during the session. Read Riders/Heroes reference notes and selected
  source for method and platform boundaries; adopted no outside code.
- Created a private GitHub repository and installed game-data guard/hooks before
  the first source-only commit. Ignored game files, builds and local handoff notes.
- Copied the user's ISO without changing it; original and copy SHA-1 matched
  [confirmed]. Identified ULES01456 version 1.00 and decrypted the main EBOOT
  locally with an external tool kept outside the repo.
- Main ELF is ET_EXEC at `0x08804040`, with 228 imports in 25 libraries [confirmed].
  Found 11 ELF archive members sharing `0x08A81660` [confirmed]; likely overlays,
  loader/lifetime not proved. Corrected archive size decoding; all 950 declared
  gzip streams decompressed with matching lengths [confirmed].
- Wrote recon, import identifiers and expanded reuse research. Existing US
  TTTRecomp is a specific reference, not a validated port of this European copy.
- Guard: 15 synthetic checks passed, including actual hooks and deleted-history
  rejection. Installed hook passed on staged project files [confirmed].
- No game/runtime implementation, generated game C, boot or gameplay test.
  Phase 1 proposed; stopped for the user's next instruction. Nothing playable
  accepted or released.
