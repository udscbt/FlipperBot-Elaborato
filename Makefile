TEX=pdflatex

pdf: ElaboratoFinale.tex ElaboratoFinale.bbl include/ code tab
	$(TEX) ElaboratoFinale.tex
	cat ElaboratoFinale.log | grep -n --color Warning

ElaboratoFinale.bbl: ElaboratoFinale.tex biblio.bib
	$(TEX) ElaboratoFinale
	bibtex ElaboratoFinale

include/robot-cpp.tex: include/code/robot.cpp
	./include/code/do include/code/robot.cpp schemo

include/server-py.tex: include/code/server.py
	./include/code/do include/code/server.py python

include/display-ebnf.tex: include/code/display.ebnf
	pygmentize -f latex -l ebnf -P texcomments=True include/code/display.ebnf > include/display-ebnf.tex
	sed -i s/REFTABLE/\\\\autoref{table:dispsym}/g include/display-ebnf.tex

code: include/robot-cpp.tex include/server-py.tex include/display-ebnf.tex

include/tab_dispsymb.tex: include/code/display.sym
	./include/code/sym2sseg.py include/code/display.sym include/tab_dispsymb.tex

tab: include/tab_dispsymb.tex

clean:
	-rm tmp.*
	-rm *.dvi
	-rm *.aux
	-rm *.log
	-rm *.toc
	-rm *.lof
	-rm *.lot
	-rm *.blg
	-rm *.bbl
	-rm *.out
	-rm img/*converted-to.pdf
