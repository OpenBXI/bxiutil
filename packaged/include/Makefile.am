
#file to be installed
cffi_files=\
		   bxi/util/version.h\
		   bxi/util/misc.h\
		   bxi/util/rng.h\
		   bxi/util/map.h\
		   bxi/util/stretch.h\
		   bxi/util/kvl.h\
		   bxi/util/vector.h

#file to be install
nobase_include_HEADERS = \
						 ${cffi_files}\
						 bxi/util/kvl_types.h

if HAVE_PYTHON
MODULE_NAME=util
../lib/bxi/${MODULE_NAME}_cffi_def.py: $(cffi_files)
	mkdir -p $$(dirname $@)
	for i in $^; do \
		echo "#include <$$i>";\
		done > cffi.h
	C_INCLUDE_PATH="$(top_srcdir)/packaged/include:$(abs_top_builddir)/packaged/include:\
	${C_INCLUDE_PATH}" \
	$(top_srcdir)/h2py.sh $$(echo ${CFLAGS}|sed 's/-g[0-9]\?\>//g') cffi.h $@;\
	rm cffi.h


all-local: ../lib/bxi/${MODULE_NAME}_cffi_def.py

.PHONY: ../lib/bxi/${MODULE_NAME}_cffi_def.py

CLEANFILES=../lib/bxi/${MODULE_NAME}_cffi_def.py
endif
