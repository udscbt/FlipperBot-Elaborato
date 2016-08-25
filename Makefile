pdf: ElaboratoFinale.tex ElaboratoFinale.bbl include/
	pdflatex -shell-escape ElaboratoFinale.tex

ElaboratoFinale.bbl: ElaboratoFinale.tex Biblio_LaTeX.bib
	pdflatex -shell-escape ElaboratoFinale
	bibtex ElaboratoFinale

clean:
	-rm tmp.*
	-rm *.dvi
	-rm *.aux
	-rm *.log
	-rm *.toc
	-rm *.blg
	-rm *.bbl
	-rm *.out
