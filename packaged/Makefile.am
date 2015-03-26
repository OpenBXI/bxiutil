lib_LTLIBRARIES=lib/libbxiutil.la

ACLOCAL_AMFLAGS=-I m4


#path of the include dir for this library
bxi_includedir=$(includedir)/bxi


lib_libbxiutil_la_SOURCES=\
		  $(include_HEADERS)\
		  src/version.c\
		  src/rng.c\
		  src/map.c\
		  src/misc.c\
		  src/vector.c\
		  src/stretch.c\
		  src/kvl.c

lib_libbxiutil_la_LDFLAGS=\
					 $(ZMQ_LIBS)\
					 $(UUID_LIBS)

lib_libbxiutil_la_CFLAGS=\
					 -I$(top_srcdir)/packaged/include\
					 $(ZMQ_CFLAGS)\
					 $(UUID_CFLAGS)\
					 $(LIB_CFLAGS)

lib_libbxiutil_la_LIBADD=$(LIB_LIBS)


#########################  PYTHON  ######################

SUBDIRS=include lib doc

#########################################################

#########################  FLEX  ######################

AM_LFLAGS=-Cf\
		  --skel=$(srcdir)/src/bxi_lex.skl

.l.c:
	$(LEX) $(AM_LFLAGS) $(LFLAGS) -o $@ $<

#########################################################
