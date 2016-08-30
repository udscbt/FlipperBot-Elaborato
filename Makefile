TEX=pdflatex

pdf: ElaboratoFinale.tex ElaboratoFinale.bbl include/ code
	$(TEX) ElaboratoFinale.tex

ElaboratoFinale.bbl: ElaboratoFinale.tex biblio.bib
	$(TEX) ElaboratoFinale
	bibtex ElaboratoFinale

include/robot-cpp.tex: include/code/robot.cpp
	./include/code/do include/code/robot.cpp schemo

include/server-py.tex: include/code/server.py
	./include/code/do include/code/server.py python

code: include/robot-cpp.tex include/server-py.tex

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
