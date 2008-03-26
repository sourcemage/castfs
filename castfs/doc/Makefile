
SECTION=8

# The following two are due to my being inept at using automake.
PREFIX=/usr
DATAROOTDIR=${PREFIX}/share

all:	castfs.${SECTION} castfs.info

castfs.${SECTION}:	castfs.tex
	latex2man -M castfs.tex castfs.${SECTION}

castfs.info:	castfs.tex
	latex2man -T castfs.tex castfs.info

install:
	cp -v castfs.${SECTION} ${DATAROOTDIR}/man/man${SECTION}
#	cp -v castfs.${SECTION} /usr/share/man/man${SECTION}

distclean:	clean

clean:
	-rm castfs.{info,${SECTION}}
