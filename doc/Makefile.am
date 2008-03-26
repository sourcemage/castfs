
SECTION=8

all:	castfs.${SECTION} castfs.info

castfs.${SECTION}:	castfs.tex
	latex2man -M castfs.tex castfs.${SECTION}

castfs.info:	castfs.tex
	latex2man -T castfs.tex castfs.info

install:
	cp -v castfs.${SECTION} ${DATAROOTDIR}/man/man${SECTION}
#	cp -v castfs.${SECTION} /usr/share/man/man${SECTION}

clean:
	rm castfs.{info,${SECTION}}
