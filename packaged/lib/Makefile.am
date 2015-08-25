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
							bxi/util/cffi_h.py


#File present inside build and source folder should be install with a special rule
bxiutildir=$(pythondir)/bxi/util
bxiutil_PYTHON=\
		   $(srcdir)/bxi/util/__init__.py\
		   /bxi/util/nodeset.py

dist-hook: ${bxiutil_PYTHON}
	cp ${bxiutil_PYTHON} $(distdir)/bxi/util

# For generation
all-local: $(nodist_nobase_python_PYTHON) $(abs_builddir)/bxi/util/__init__.py

clean-local:
	if test $(srcdir) != $(builddir);\
		then\
		rm -f $(builddir)/bxi/util/__init__.py $(builddir)/bxi/__init__.py;\
	fi

#
##
#
CLEANFILES=bxi/util/cffi_h.py
endif
