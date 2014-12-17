ACLOCAL_AMFLAGS = -I m4

AM_LFLAGS=-Cf\
		  --skel=$(abs_srcdir)/src/bxi_kvp.skl


lib_LTLIBRARIES=lib/libbxiutil.la

#path of the include dir for this library
bxi_includedir=$(includedir)/bxi


lib_libbxiutil_la_SOURCES=\
		  $(include_HEADERS)\
		  src/rng.c\
		  src/map.c\
		  src/misc.c\
		  src/vector.c\
		  src/tp.c\
		  src/kvl.l

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

SUBDIRS=lib include doc

#########################################################
