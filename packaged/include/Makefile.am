
cffi_files=\
		   bxi/util/misc.h\
		   bxi/util/rng.h\
		   bxi/util/map.h\
		   bxi/util/tp.h\
		   bxi/util/vector.h

#file to be install
nobase_include_HEADERS = \
						 ${cffi_files}\
						 bxi/util/kvl_types.h\
						 bxi/util/kvl.h

../lib/bxi/util/cffi_h.py: $(cffi_files) 
	mkdir -p $$(dirname $@)
	touch  $$(dirname $@)/__init__.py
	touch  $$(dirname $$(dirname $@))/__init__.py
	for i in $^; do \
		echo "#include <$$i>";\
		done > cffi.h
	C_INCLUDE_PATH=$(top_srcdir)/packaged/include $(top_srcdir)/h2py.sh cffi.h $@ 


all-local: ../lib/bxi/util/cffi_h.py

.PHONY: ../lib/bxi/util/cffi_h.py
