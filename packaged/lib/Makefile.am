#
##
#

if HAVE_PYTHON
$(abs_builddir)/bxi/__init__.py:
	touch  $@

nobase_python_PYTHON=\
					 bxi/util/nodeset.py\
					 bxi/util/task.py\
					 bxi/util/interface.py

MODULE_NAME=util
include Makefile_spec
CLEANFILES=${nobase_nodist_python_PYTHON}
endif
