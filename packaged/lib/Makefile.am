#
##
#

if HAVE_PYTHON
$(abs_builddir)/bxi/__init__.py:
	touch  $@

$(abs_builddir)/bxi/util/__init__.py: $(abs_builddir)/bxi/__init__.py
	touch  $@



# For installation
nobase_nodist_python_PYTHON=\
							bxi/util/version.py\
							bxi/util/cffi_h.py

nobase_python_PYTHON=\
							bxi/util/nodeset.py\
							bxi/util/task.py\
							bxi/util/interface.py

#File present inside build and source folder should be install with a special rule
bxiutildir=$(pythondir)/bxi/util
bxiutil_PYTHON=\
		   $(srcdir)/bxi/util/__init__.py

dist-hook: ${bxiutil_PYTHON}
	cp ${bxiutil_PYTHON} $(distdir)/bxi/util
	cp $(srcdir)/bxi/__init__.py $(distdir)/bxi/

# For generation
all-local: $(nodist_nobase_python_PYTHON) $(abs_builddir)/bxi/util/__init__.py

clean-local:
	if test $(srcdir) != $(builddir);\
		then\
		rm -f $(builddir)/bxi/util/__init__.py $(builddir)/bxi/__init__.py;\
	fi

bxi/%/version.py:${abs_top_builddir}/version.py $(abs_top_builddir)/packaged/lib/bxi/util
	cp $< $@
BUILT_SOURCES=bxi/util/version.py
#
##
#
CLEANFILES=bxi/util/cffi_h.py
endif
