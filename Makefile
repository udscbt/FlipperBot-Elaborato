TEX=pdflatex

pdf: ElaboratoFinale.tex ElaboratoFinale.bbl include/ code
	$(TEX) ElaboratoFinale.tex

ElaboratoFinale.bbl: ElaboratoFinale.tex biblio.bib
	$(TEX) ElaboratoFinale
	bibtex ElaboratoFinale

code: include/code/network.cpp
	./include/code/do include/code/network.cpp schemo

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
