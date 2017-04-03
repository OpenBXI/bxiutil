#
##
#

if HAVE_PYTHON
MODULE_NAME=util
$(abs_builddir)/bxi/__init__.py:
	touch  $@

$(abs_builddir)/bxi/${MODULE_NAME}/__init__.py: $(abs_builddir)/bxi/__init__.py
	touch  $@

bxi/${MODULE_NAME}/cffi_h.py:bxi/${MODULE_NAME}/builder.py bxi/${MODULE_NAME}_cffi_def.py \
	$(abs_builddir)/bxi/__init__.py
	C_INCLUDE_PATH="$(top_srcdir)/packaged/include:$(abs_top_builddir)/packaged/include:\
	${C_INCLUDE_PATH}" \
	PYTHONPATH="$(top_srcdir)/packaged/lib:$(abs_top_builddir)/packaged/lib:${PYTHONPATH}" \
	$<


# For installation
nobase_nodist_python_PYTHON=\
							bxi/${MODULE_NAME}/version.py\
							bxi/${MODULE_NAME}_cffi_def.py\
							bxi/${MODULE_NAME}/cffi_h.py

nobase_python_PYTHON=\
							bxi/util/nodeset.py\
							bxi/util/task.py\
							bxi/util/interface.py

#File present inside build and source folder should be install with a special rule
bxiutildir=$(pythondir)/bxi/${MODULE_NAME}
bxiutil_PYTHON=\
		   $(srcdir)/bxi/${MODULE_NAME}/__init__.py

dist-hook: ${bxiutil_PYTHON}
	cp ${bxiutil_PYTHON} $(distdir)/bxi/${MODULE_NAME}

# For generation
all-local:\
	$(nodist_nobase_python_PYTHON) \
	$(abs_builddir)/bxi/${MODULE_NAME}/__init__.py \
	bxi/${MODULE_NAME}/cffi_h.py

clean-local:
	if test $(srcdir) != $(builddir);\
		then\
		rm -f $(builddir)/bxi/${MODULE_NAME}/__init__.py $(builddir)/bxi/__init__.py \
			$(builddir)/bxi/${MODULE_NAME}/cffi_h.py;\
	fi

$(abs_top_builddir)/packaged/lib/bxi/${MODULE_NAME}:
	mkdir -p $@

bxi/%/version.py:${abs_top_builddir}/version.py $(abs_top_builddir)/packaged/lib/bxi/${MODULE_NAME}
	cp $< $@
BUILT_SOURCES=bxi/${MODULE_NAME}/version.py
#
##
#
CLEANFILES=bxi/${MODULE_NAME}/cffi_h.py
endif
