MAINTAINERCLEANFILES = Makefile.in .deps *~

AM_CPPFLAGS = -I ../include

EXTRA_PROGRAMS = testcopyfile \
		 testfiletree \
		 testmkdirrec

COMMON_SOURCES = ../src/hash.c \
		 ../src/utils.c \
		 ../src/rbtree.c

testcopyfile_SOURCES = copyfile.c \
		       ${COMMON_SOURCES}

testfiletree_SOURCES = filetree.c \
		       ${COMMON_SOURCES}

testmkdirrec_SOURCES = mkdirrec.c \
		       ${COMMON_SOURCES}

