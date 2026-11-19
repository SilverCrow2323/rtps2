#!/usr/bin/env python3
"""Host-only transactional reference engine. Never targets a real PS2 mount."""
from __future__ import annotations
import hashlib, json, os, shutil, time
from pathlib import Path, PurePosixPath

PROTECTED = {"BOOT", "SYS-CONF", "SYSTEM", "OSDSYS", "FMCB", "FHDB", "PSBBN", "PS2BBL"}
TERMINAL = {"COMMITTED", "ROLLED_BACK", "FAILED"}

class TransactionError(RuntimeError): pass

def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open('rb') as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b''): h.update(chunk)
    return h.hexdigest()

def safe_rel(value: str) -> Path:
    if not isinstance(value, str) or not value or '\x00' in value or '\\\\' in value:
        raise TransactionError(f"unsafe relative path: {value!r}")
    p = PurePosixPath(value)
    if p.is_absolute() or any(x in ('', '.', '..') for x in p.parts):
        raise TransactionError(f"unsafe relative path: {value!r}")
    if any(x.upper() in PROTECTED or x.upper().startswith(('BOOT.', 'SYS-CONF.')) for x in p.parts):
        raise TransactionError(f"protected system path: {value!r}")
    return Path(*p.parts)

class Transaction:
    """Sandbox reference: durable journal, exclusive lock and constrained recovery."""
    def __init__(self, root: Path, state_dir: Path):
        self.root, self.state_dir = root.resolve(), state_dir.resolve()
        if self.root == self.state_dir or self.root in self.state_dir.parents or self.state_dir in self.root.parents:
            raise TransactionError('sandbox root and transaction state must be separate trees')
        self.root.mkdir(parents=True, exist_ok=True)
        self.state_dir.mkdir(parents=True, exist_ok=True)
        self.journal = self.state_dir / 'journal.json'
        self.backups = self.state_dir / 'backups'
        self.lock = self.state_dir / 'transaction.lock'

    def _exclusive(self):
        try: fd = os.open(self.lock, os.O_CREAT | os.O_EXCL | os.O_WRONLY, 0o600)
        except FileExistsError as e: raise TransactionError('another transaction holds the lock; inspect before removing stale lock') from e
        os.write(fd, f'{os.getpid()}\n'.encode()); os.fsync(fd); return fd

    def _unlock(self, fd):
        os.close(fd)
        try: self.lock.unlink()
        except FileNotFoundError: pass

    def _write(self, data):
        tmp = self.journal.with_suffix('.tmp')
        with tmp.open('w', encoding='utf-8') as f:
            json.dump(data, f, indent=2, sort_keys=True); f.write('\n'); f.flush(); os.fsync(f.fileno())
        os.replace(tmp, self.journal)
        try:
            dfd = os.open(self.state_dir, os.O_RDONLY); os.fsync(dfd); os.close(dfd)
        except OSError: pass

    def _target(self, rel: str) -> Path:
        p = safe_rel(rel); cur = self.root
        for part in p.parts:
            cur = cur / part
            if cur.is_symlink(): raise TransactionError(f'target path traverses symlink: {rel}')
        resolved = cur.resolve(strict=False)
        if self.root not in resolved.parents: raise TransactionError(f'target escapes sandbox: {rel}')
        return cur

    def _validate_manifest(self, manifest, payload):
        files = manifest.get('files') if isinstance(manifest, dict) else None
        if not isinstance(files, list) or not files: raise TransactionError('manifest.files must be a non-empty list')
        entries, seen = [], set(); payload = payload.resolve()
        for item in files:
            if not isinstance(item, dict): raise TransactionError('each file entry must be an object')
            rel = safe_rel(item.get('path')); source_rel = safe_rel(item.get('source'))
            if rel.as_posix() in seen: raise TransactionError('duplicate target path')
            seen.add(rel.as_posix())
            src = (payload / source_rel).resolve()
            if payload not in src.parents or not src.is_file(): raise TransactionError('payload source missing or escapes payload')
            digest = str(item.get('sha256', '')).lower()
            if len(digest) != 64 or any(c not in '0123456789abcdef' for c in digest) or sha256(src) != digest:
                raise TransactionError(f'payload hash mismatch or invalid SHA-256: {rel}')
            dst = self._target(rel.as_posix())
            if dst.exists() and (not dst.is_file() or dst.is_symlink()): raise TransactionError(f'target is not a regular file: {rel}')
            entries.append({'path':rel.as_posix(),'source':str(src),'sha256':digest,'existed':dst.exists(),'backup':None})
        return entries

    def install(self, manifest: dict, payload: Path, fail_after: int | None = None):
        fd = self._exclusive()
        try: return self._install_locked(manifest, payload, fail_after)
        finally: self._unlock(fd)

    def _install_locked(self, manifest, payload, fail_after):
        if self.journal.exists():
            old = json.loads(self.journal.read_text())
            if old.get('state') not in TERMINAL: raise TransactionError('unfinished transaction exists; recover first')
        entries = self._validate_manifest(manifest, payload)
        txid = f'nxa-{time.time_ns()}'; self.backups.mkdir(parents=True, exist_ok=True)
        record = {'schema':1,'id':txid,'state':'PREPARING','root':str(self.root),'entries':entries,'completed':0}
        self._write(record); record['state']='APPLYING'; self._write(record)
        try:
            for index, entry in enumerate(entries, 1):
                dst = self._target(entry['path']); dst.parent.mkdir(parents=True, exist_ok=True)
                if entry['existed']:
                    backup = self.backups / f'{txid}-{index}.bak'; shutil.copy2(dst, backup)
                    entry['backup'] = str(backup); self._write(record)
                temp = dst.with_name(dst.name + f'.nexus-{txid}.tmp')
                shutil.copyfile(entry['source'], temp)
                with temp.open('rb') as f: os.fsync(f.fileno())
                if sha256(temp) != entry['sha256']: raise TransactionError('temporary read-back hash mismatch')
                os.replace(temp, dst); record['completed'] = index; self._write(record)
                if fail_after == index: raise TransactionError('injected failure for rollback test')
            record['state']='VERIFYING'; self._write(record)
            for entry in entries:
                if sha256(self._target(entry['path'])) != entry['sha256']: raise TransactionError('target verification failed')
            record['state']='COMMITTED'; self._write(record); return record
        except Exception:
            record['state']='ROLLING_BACK'; self._write(record)
            self._restore(record); record['state']='ROLLED_BACK'; self._write(record); raise

    def _restore(self, record):
        if Path(record.get('root','')).resolve() != self.root: raise TransactionError('journal root mismatch; refusing recovery')
        for entry in reversed(record.get('entries', [])):
            dst = self._target(entry['path'])
            backup_value = entry.get('backup')
            if backup_value:
                backup = Path(backup_value).resolve()
                if self.backups.resolve() not in backup.parents or not backup.is_file(): raise TransactionError('journal backup path invalid; refusing recovery')
                dst.parent.mkdir(parents=True, exist_ok=True); shutil.copy2(backup, dst)
            elif not entry.get('existed'):
                try: dst.unlink()
                except FileNotFoundError: pass

    def recover(self):
        fd = self._exclusive()
        try:
            if not self.journal.exists(): return {'state':'CLEAN'}
            record = json.loads(self.journal.read_text())
            if record.get('state') in TERMINAL: return record
            record['state']='ROLLING_BACK'; self._write(record); self._restore(record)
            record['state']='ROLLED_BACK'; self._write(record); return record
        finally: self._unlock(fd)
