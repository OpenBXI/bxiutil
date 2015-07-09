lib_LTLIBRARIES=lib/libbxiutil.la
noinst_LTLIBRARIES=\
				   lib/libbxiutil_kvl.la\
				   lib/libbxiutil_common.la


ACLOCAL_AMFLAGS=-I m4


#path of the include dir for this library
bxi_includedir=$(includedir)/bxi


lib_libbxiutil_la_SOURCES=

lib_libbxiutil_common_la_SOURCES=\
		  $(include_HEADERS)\
		  src/version.c\
		  src/rng.c\
		  src/map.c\
		  src/misc.c\
		  src/vector.c\
		  src/stretch.c

lib_libbxiutil_kvl_la_SOURCES=\
		  src/kvl.c

lib_libbxiutil_common_la_LDFLAGS=\
					 $(ZMQ_LIBS)

lib_libbxiutil_kvl_la_CFLAGS=\
					 -I$(top_srcdir)/packaged/include

lib_libbxiutil_common_la_CFLAGS=\
					 -I$(top_srcdir)/packaged/include\
					 $(ZMQ_CFLAGS)\
					 $(LIB_CFLAGS)

lib_libbxiutil_la_LIBADD=\
					 $(ZMQ_LIBS)\
					 lib/libbxiutil_kvl.la\
					 lib/libbxiutil_common.la\
					 $(LIB_LIBS)

TESTS=\
	  doc/examples/bximap\
	  doc/examples/bxistretch

check_PROGRAMS=\
			  doc/examples/bximap\
			  doc/examples/bxistretch

all-local:
	mkdir -p doc/examples


doc_examples_bximap_LDADD=\
						 lib/libbxiutil.la

#########################  PYTHON  ######################

SUBDIRS=include lib doc

EXTRA_DIST=\
		   doc/examples/bximap.c\
		   doc/examples/bxistretch.c

#########################################################

#########################  FLEX  ######################

if BXIFLEX_UNCOMPATIBLE
lib_libbxiutil_kvl_la_CFLAGS+=-Wno-error
else
AM_LFLAGS=-Cf\
		  --skel=$(srcdir)/src/bxi_lex.skl
endif

.l.c:
	$(LEX) $(AM_LFLAGS) $(LFLAGS) -o $@ $<

#########################################################
