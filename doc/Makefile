
SECTION=8

# The following two are due to my being inept at using automake.
PREFIX=/usr
DATAROOTDIR=${PREFIX}/share

all:
	touch castfs.${SECTION}

docs:	castfs.${SECTION} castfs.info

castfs.${SECTION}:	castfs.tex
	latex2man -M castfs.tex castfs.${SECTION}

castfs.info:	castfs.tex
	latex2man -T castfs.tex castfs.info

install:
	install -m 0644 castfs.8 ${DATAROOTDIR}/man/man8
	install -m 0644 castfs.info ${DATAROOTDIR}/info
	install-info --info-dir=/usr/share/info \
			${INSTALL_ROOT}/usr/share/info/castfs.info

distclean:	clean

clean:
	echo "Nothing to clean..."
