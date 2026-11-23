#!/usr/bin/env python3
"""Fail closed on game material, generated output or private data in Git.

Default / --staged: inspect every blob in the index, not working-tree copies.
--history: inspect every path in every commit reachable from refs and HEAD.
This is a safety net; human review remains necessary for disguised text assets.
"""
import argparse
from pathlib import PurePosixPath
import re
import subprocess
import sys

MAX_BYTES = 512 * 1024
BLOCKED_DIRS = {
    'baserom', 'build', 'extracted', 'generated', 'gen', 'asm', 'saves',
    'keys', 'assets_out', '.local', '.codex', '.agents', '.claude',
}
BLOCKED_EXTS = set(('iso cso zso pbp prx elf bin sfo at3 pmf pgf gim gmo pak '
                    'sav keys key exe dll o obj a lib nro nso nsp nca xci '
                    'png jpg jpeg bmp tga wav mp3 ogg mp4 zip 7z rar gz '
                    's asm map log raw dat db sqlite pyc').split())
PRIVATE_NAMES = {'ai_handoff.md', 'claude.local.md', '.env'}
GAME_SHA1 = {
    '99b9075c67e17a2f99716f5d0b356e34623bd385',
    '272e18eb11b022067a7f592b2cd9f0f18ee6ae9d',
    '3c2c8443c6f36af17a0a5571d77696ace3b452d1',
    'd1d930da28e48dd4ba49d6659035d62c803a1edd',
}
HOME_PATH = re.compile(r'(?i)(?:[a-z]:[\\/]' + r'users[\\/]|/(?:home|Users)/)(?!<)[^\\/\s]+')
DRIVE_PATH = re.compile(r'(?i)\b[a-z]:[\\/]')
EMAIL = re.compile(r'\b[\w.+-]+' + '@' + r'[\w.-]+\.[a-zA-Z]{2,}\b')
SECRET = re.compile(r'(?i)\b\w*(?:ke' + r'y|kek|token|password)\w*\s*[:=]\s*[\x22\x27]?[0-9a-f]{32,}')
HEX_TABLE = re.compile(r'(?i)(?:\b0x[0-9a-f]{2}\s*,\s*){16,}')
ASM = re.compile(r'(?im)^\s*(?:/\*\s*)?(?:0x)?[0-9a-f]{8}(?:\s*[:/]\s*|\s+)[0-9a-f]{8}\b')
GENERATED_WORD = 'genera' + 'ted'
GENERATED = re.compile(r'(?i)(?:automatically ' + GENERATED_WORD + '|' + GENERATED_WORD
                       + r' by).{0,100}(?:recomp|allegrex|mips)|(?:recomp|allegrex).{0,100}'
                       + GENERATED_WORD)


def git(*args):
    return subprocess.run(['git', *args], check=True, capture_output=True).stdout


def inspect(path, mode, oid):
    reasons = []
    lower = path.lower()
    parts = PurePosixPath(lower).parts
    if any(part in BLOCKED_DIRS for part in parts):
        reasons.append('game-data, build or local-only directory')
    if PurePosixPath(lower).suffix.lstrip('.') in BLOCKED_EXTS:
        reasons.append('forbidden game-data or binary file type')
    if parts[-1] in PRIVATE_NAMES or parts[-1].startswith('.env.'):
        reasons.append('private local file')
    if re.search(r'(?i)(?:^|/)(?:.*_funcs_\d+\.[ch]|.*_recomp\.[ch]|.*\.disasm)$', path):
        reasons.append('generated game output')
    if mode not in ('100644', '100755'):
        return reasons + ['symlink or submodule cannot be audited as source']
    size = int(git('cat-file', '-s', oid))
    if size > MAX_BYTES:
        return reasons + ['file exceeds source-only size limit']
    data = git('cat-file', 'blob', oid)
    import hashlib
    if hashlib.sha1(data).hexdigest() in GAME_SHA1:
        reasons.append('known game file content')
    if b'\0' in data or data.startswith((b'~PSP', b'\x7fELF', b'CISO', b'PSMF', b'RIFF', b'PK\x03\x04', b'\x1f\x8b')):
        return reasons + ['binary or game container content']
    try:
        text = data.decode('utf-8')
    except UnicodeDecodeError:
        return reasons + ['non-UTF-8 content']
    if HOME_PATH.search(text) or DRIVE_PATH.search(text):
        reasons.append('personal or machine-specific path')
    if EMAIL.search(text):
        reasons.append('email address')
    if SECRET.search(text) or HEX_TABLE.search(text):
        reasons.append('possible secret or key byte table')
    if ASM.search(text) or GENERATED.search(text):
        reasons.append('possible disassembly or recompiled game code')
    return reasons


def entries(history):
    if not history:
        records = git('ls-files', '--stage', '-z').split(b'\0')
        for record in records:
            if not record:
                continue
            metadata, path = record.split(b'\t', 1)
            mode, oid, stage = metadata.decode().split()
            if stage != '0':
                raise ValueError('unmerged index; resolve before committing')
            yield path.decode('utf-8'), mode, oid
        return
    commits = git('rev-list', '--all', 'HEAD').decode().splitlines()
    seen = set()
    for commit in commits:
        for record in git('ls-tree', '-r', '-z', commit).split(b'\0'):
            if not record:
                continue
            metadata, path = record.split(b'\t', 1)
            mode, kind, oid = metadata.decode().split()
            entry = (path.decode('utf-8'), mode, oid)
            if entry not in seen:
                seen.add(entry)
                yield entry


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    group = parser.add_mutually_exclusive_group()
    group.add_argument('--staged', action='store_true')
    group.add_argument('--history', action='store_true')
    args = parser.parse_args()
    try:
        root = git('rev-parse', '--show-cdup').decode().strip() or '.'
        import os
        os.chdir(root)
        count = bad = 0
        for path, mode, oid in entries(args.history):
            count += 1
            for reason in inspect(path, mode, oid):
                print(f'REFUSED {path!r}: {reason}')
                bad += 1
        print(f'Game-data guard: {count} file versions checked; {bad} problems.')
        return bool(bad)
    except (OSError, ValueError, UnicodeError, subprocess.CalledProcessError) as exc:
        print(f'Game-data guard could not finish: {type(exc).__name__}', file=sys.stderr)
        return 1


if __name__ == '__main__':
    sys.exit(main())
