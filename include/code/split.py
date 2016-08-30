#!/usr/bin/env python
# USAGE: ./split.py COMMENT START END FILE OUTDIR
import sys
import re
import os.path

(BASE, START, END, FILE, OUTDIR) = sys.argv[1:]

COMMENT = BASE+BASE
START = BASE+START
END = BASE+END

f = open(FILE, "r")
content = f.read()
f.close()

blocks = {}
block = None
linen = 0
for line in content.split("\n"):
  oldline = line
  if line.startswith(BASE):
    if line.startswith(COMMENT):
      continue
  else:
    linen = linen + 1
  if COMMENT in line:
    line = line[:line.find(COMMENT)]
  while line is not None and (line.find(START) >= 0 or line.find(END) >= 0):
    s = line.find(START)
    e = line.find(END)
    if block is None:
      if e >= 0 and e < s:
        raise Exception("Block stopped before being started")
      block = [line[s+len(START):None if e < 0 else e] if oldline != START else None]
      blocks[linen + (1 if block == [None] else 0)] = block
      line = None if e < 0 else line[e:]
    else:
      if s >= 0 and s < e:
        raise Exception("Block started before the previous stopped")
      block.append(line[:e])
      block = None
      line = line[e+len(END):]
  if block is not None:
    block.append(line)

for l, b in blocks.items():
  f = open(os.path.join(OUTDIR, "{}.tmp".format(l)), "w")
  f.write("\n".join([line for line in b if line is not None]))
  f.close()
