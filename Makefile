TEX=pdflatex

pdf: ElaboratoFinale.tex ElaboratoFinale.bbl include/
	$(TEX) ElaboratoFinale.tex

ElaboratoFinale.bbl: ElaboratoFinale.tex biblio.bib
	$(TEX) ElaboratoFinale
	bibtex ElaboratoFinale

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
