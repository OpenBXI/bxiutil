#
##
#

$(abs_builddir)/bxi/__init__.py:
	touch  $@

$(abs_builddir)/bxi/util/__init__.py: $(abs_builddir)/bxi/__init__.py
	touch  $@



# For installation
nobase_python_PYTHON=\
                  bxisignal.py\
                  bxidaemon.py\
                  bxi/util/cffi_h.py


#File present inside build and source folder should be install with a special rule
bxiutildir=$(pythondir)/bxi/util
bxiutil_PYTHON=\
		   $(srcdir)/bxi/util/__init__.py 

# For generation
all-local: $(nobase_python_PYTHON) $(abs_builddir)/bxi/util/__init__.py

clean-local:
	if test $(srcdir) != $(builddir);\
		then\
		rm -f $(builddir)/bxi/util/__init__.py $(builddir)/bxi/__init__.py;\
	fi

#
##
#
CLEANFILES=bxi/util/cffi_h.py
