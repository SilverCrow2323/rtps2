#!/usr/bin/env python3
import argparse, json
from pathlib import Path
from nexus_txn.engine import Transaction
p=argparse.ArgumentParser(description='NEXUS host-only transaction simulator')
p.add_argument('--root',required=True,type=Path); p.add_argument('--state',required=True,type=Path)
s=p.add_subparsers(dest='cmd',required=True)
i=s.add_parser('install'); i.add_argument('--manifest',required=True,type=Path); i.add_argument('--payload',required=True,type=Path); i.add_argument('--fail-after',type=int)
s.add_parser('recover')
a=p.parse_args(); tx=Transaction(a.root,a.state)
if a.cmd=='recover': out=tx.recover()
else: out=tx.install(json.loads(a.manifest.read_text()),a.payload,a.fail_after)
print(json.dumps(out,indent=2))
