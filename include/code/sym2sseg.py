#!/usr/bin/env python
import sys
from collections import OrderedDict

f = open(sys.argv[1], "r")
content = f.read()
f.close()

syms = OrderedDict()
for line in content.split("\n"):
  try:
    sym, segs = line.split("\t")
    syms[sym] = {x: 'on' if x in segs else 'off' for x in "abcdefg."}
  except:
    print("Warning: {} is not well formatted".format(repr(line)))

f = open(sys.argv[2], "w")
for sym, segs in syms.items():
  f.write((r"\Verb+%s+ & " % sym) + (r"""
\begin{tikzpicture}
  [
    baseline=(base.base),
    x=0.2em, y=0.2em,
    segment/.style = {ultra thick},
    on/.style = {black},
    off/.style = {lightgray}
  ]
  \node (base) at (0,0.5) {};
  \draw [segment, %(a)s] (0.5,6) -- (2.5,6);
  \draw [segment, %(b)s] (3,5.5) -- (3,3.5);
  \draw [segment, %(c)s] (3,2.5) -- (3,0.5);
  \draw [segment, %(d)s] (0.5,0) -- (2.5,0);
  \draw [segment, %(e)s] (0,2.5) -- (0,0.5);
  \draw [segment, %(f)s] (0,5.5) -- (0,3.5);
  \draw [segment, %(g)s] (0.5,3) -- (2.5,3);
  \draw [%(.)s, fill] (3,0) circle (0.25);
\end{tikzpicture} \\
""" % segs))
f.close()
