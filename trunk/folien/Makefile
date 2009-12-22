all: praese.pdf
SRC=praese.tex
praese.toc: $(SRC)
	pdflatex praese.tex
	rm praese.pdf

praese.pdf: $(SRC) praese.toc
#praese.bbl
	pdflatex praese.tex
	gnome-open praese.pdf

#praese.bbl: praese.toc praese.aux
#	bibtex praese

clean:
	rm -f *.aux *~ praese.lof praese.log praese.out praese.pdf praese.toc praese.nav praese.snm

view:
	gnome-open praese.pdf
