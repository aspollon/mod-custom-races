#!/usr/bin/env python3
"""The racial abilities of CoA Custom's races, into races.json ("coaCustom").

CoA Custom does not keep them in a data file: its server has them compiled in (src/server/shared/CoaCustomRacials.h,
race number -> up to 12 spells, taught at every login by CoaRacialSync). This script reads that table from a checkout
of CoA Custom's source (github.com/ilusixn/azerothcore-wotlk-coa, the branch of the CoA Custom version) and writes it
into races.json, where AFK Realm finds it:

  "coaCustom": { "source": "...", "racials": { "<race number>": [spell, ...] }, "scripted": [spell, ...] }

"scripted": the racials whose working needs code of CoA Custom's server (src/server/coa/EsteriaRacials/*.cpp names
them); the others are plain spell rows and work with their rows alone. "ported": src/coa_racials of this module has that
code. "rows": CoA Custom's rows for them (its files/sql/10_esteria_racials_*.sql of the server pack: spell_dbc,
spell_script_names, spell_proc), copied to data/coa-custom/racials.sql.

  python3 tools/coa_custom_racials.py <checkout of azerothcore-wotlk-coa> [label]
"""
import json
import os
import re
import sys

HERE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 1
    repo = sys.argv[1]
    label = sys.argv[2] if len(sys.argv) > 2 else os.path.basename(os.path.abspath(repo))
    header = open(os.path.join(repo, 'src', 'server', 'shared', 'CoaCustomRacials.h'), encoding='utf-8').read()
    table = {}
    for race, spells in re.findall(r'\{\s*(\d+)\s*,\s*\{([\d,\s]+)\}\s*\}', header):
        ids = [int(s) for s in spells.split(',') if s.strip() and int(s) != 0]
        if ids:
            table[str(int(race))] = ids

    # The spells the scripts of CoA Custom's server name, by the constants of its generated header and by number.
    folder = os.path.join(repo, 'src', 'server', 'coa', 'EsteriaRacials')
    names = {}
    code = ''
    for file in sorted(os.listdir(folder)):
        text = open(os.path.join(folder, file), encoding='utf-8', errors='replace').read()
        for name, value in re.findall(r'\b(SPELL_[A-Z0-9_]+)\s*=\s*(\d+)', text):
            names[name] = int(value)
        if file.endswith('.cpp'):
            code += text
    named = {value for name, value in names.items() if re.search(r'\b' + name + r'\b', code)}
    named |= {int(n) for n in re.findall(r'\b(1[1-4]\d{4,6})\b', code)}
    wanted = {s for spells in table.values() for s in spells}
    scripted = sorted(wanted & named)

    path = os.path.join(HERE, 'races.json')
    data = json.load(open(path, encoding='utf-8'))
    # src/coa_racials holds CoA Custom's code for them (taken over from the same branch): "ported" tells AFK Realm so.
    ported = os.path.isfile(os.path.join(HERE, 'src', 'coa_racials', 'race_racials.cpp'))
    data['coaCustom'] = {'source': label, 'racials': dict(sorted(table.items(), key=lambda kv: int(kv[0]))), 'scripted': scripted,
                         'ported': ported, 'rows': 'data/coa-custom/racials.sql'}
    text = json.dumps(data, ensure_ascii=False, indent=1)
    open(path, 'w', encoding='utf-8', newline='\n').write(text + '\n')
    print('%d races, %d racials, %d of them need code of CoA Custom\'s server' % (len(table), len(wanted), len(scripted)))
    return 0


if __name__ == '__main__':
    sys.exit(main())
