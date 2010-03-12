all: documentation.pdf
SRC=appendix-usage.tex ch1-data-acquirement.tex documentation.tex ch0-intro.tex ch2-kernel-trouble.tex ch3-memory-access.tex ch4-rootkit-analysis.tex

documentation.toc: $(SRC)
	pdflatex documentation.tex
	rm documentation.pdf

documentation.pdf: $(SRC) documentation.toc
#documentation.bbl
	pdflatex documentation.tex
	gnome-open documentation.pdf

#documentation.bbl: documentation.toc documentation.aux
#	bibtex documentation

clean:
	rm -f *.aux *~ documentation.lof documentation.log documentation.out documentation.pdf documentation.toc

view:
	gnome-open documentation.pdf
