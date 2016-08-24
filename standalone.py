#!/usr/bin/env python

import sys

INCLUDE_DIR="include"

template=r"""
\documentclass[tikz]{standalone}

\usepackage[english]{babel}
\usepackage[utf8]{inputenc}
\usepackage{fancyhdr}
\usepackage[official]{eurosym}
%Added
\usepackage{amssymb,amsmath}
\usepackage{gensymb}
\usepackage{titlesec}
\usepackage{graphicx}
\usepackage{subcaption}
\usepackage[nottoc,numbib]{tocbibind}
\usepackage{wrapfig}
\usepackage[export]{adjustbox}
%%%%% FABIO FA BRUTTE COSE
\newcommand{\includeDir}{include}
\input{\includeDir/utils}
\import{fabiopkg}
%%%%% FABIO SMETTE

\begin{document}
<CONTENT>
\end{document}
"""

i = open(INCLUDE_DIR+"/"+sys.argv[1], "r")
o = open("tmp.tex", "w")
o.write(template.replace("<CONTENT>", i.read()))
i.close()
o.close()
