###############################################################################
# $Revision: 1.86 $
# $Date: 2011/07/06 16:54:26 $
###############################################################################

#TODO: define your package name
%define name bxiutil

# Bull software starts with 1.1-Bull.1.0
# For versionning policy, please see wiki:
# http://intran0x.frec.bull.fr/projet/HPC/wiki_etudes/index.php/How_to_generate_RPM#Bull_rpm_NAMING_CONVENTION
%define version 4.0.1

# Using the .snapshot suffix helps the SVN tagging process.
# Please run <your_svn_checkout>/devtools/packaged/bin/auto_tag -m
# to get the auto_tag man file
# and to understand the SVN tagging process.
# If you don't care, then, just starts with Bull.1.0%{?dist}.%{?revision}snapshot
# and run 'make tag' when you want to tag.
%define release Bull.4.2%{?dist}.%{?revision}snapshot

# Warning: Bull's continuous compilation tools refuse the use of
# %release in the src_dir variable!
%define src_dir %{name}-%{version}
%define src_tarall %{src_dir}.tar.gz

# Predefined variables:
# {%_mandir} => normally /usr/share/man (depends on the PDP)
# %{perl_vendorlib} => /usr/lib/perl5/vendor_perl/

Prefix: /etc
Prefix: /usr

# Other custom variables
%define src_conf_dir conf
%define src_bin_dir bin
%define src_lib_dir lib
%define src_doc_dir doc

%define target_conf_dir /etc/
%define target_bin_dir /usr/bin
%define target_lib_dir /usr/lib*
%define target_python_lib_dir %{python2_sitearch}
%define target_man_dir %{_mandir}
%define target_doc_dir /usr/share/doc/%{name}
# @TODO : Support cases where BXI_BUILD_{SUB,}DIR variables are not defined
#         Attempt to get tagfiles from installed doc packages
%define src_tagfiles_prefix %{?tagfiles_prefix}%{?!tagfiles_prefix:%{bxi_build_dir}}
%define src_tagfiles_suffix %{?tagfiles_suffix}%{?!tagfiles_suffix:%{bxi_build_subdir}/packaged/doc/doxygen.tag}
%define target_htmldirs_prefix ../../
%define target_htmldirs_suffix /last/

# TODO: Give your summary
Summary:	The bxiutil provides all functions and objects that are used by BXI objects
Name:		%{name}
Version:	%{version}
Release:	%{release}
Source:		%{src_tarall}
# Perform a 'cat /usr/share/doc/rpm-*/GROUPS' to get a list of available
# groups. Note that you should be in the corresponding PDP to get the
# most accurate information!
# TODO: Specify the category your software belongs to
Group:		Development/Libraries
BuildRoot:	%{_tmppath}/%{name}-root
# Automatically filled in by PDP: it should not appear therefore!
#Packager:	BXIHL <bxihl@atos.net>
Distribution:	Bull HPC

# Automatically filled in by PDP: it should not appear therefore!
#Vendor:         Bull
License:        'Bull S.A.S. proprietary : All rights reserved'
BuildArch:      x86_64
URL:            https://novahpc.frec.bull.fr

#TODO: What do you provide
Provides: %{name}

#Conflicts:

# BXI
BuildRequires: bxibase-devel >= 9.2.0
Requires: bxibase >= 9.2.0

# External
Requires: zeromq

BuildRequires: zeromq-devel
BuildRequires: flex == 2.5.37
BuildRequires: gcc
buildRequires: gcc-c++
BuildRequires: net-snmp-devel
BuildRequires: CUnit-devel

# Description (seen by rpm -qi) (No more than 80 characters)
%description
BXI utils functions

%package doc
Summary: Documentation of BXI utils functions
#BuildRequires: bxibase-doc
Requires: bxibase-doc

# Description (seen by rpm -qi) (No more than 80 characters)
%description doc
Doxygen documentation of BXI utils functions

%package devel
Summary: Header files providing the bxiutil API
Requires: %{name}
# Description (seen by rpm -qi) (No more than 80 characters)
%description devel
Header files providing the bxiutil API

%package tests
Summary: Tests for the BXI util package
Requires: %{name}

# Description (seen by rpm -qi) (No more than 80 characters)
%description tests
Tests for BXI util package

###############################################################################
# Prepare the files to be compiled
%prep
#%setup -q -n %{name}
test "x$RPM_BUILD_ROOT" != "x" && rm -rf $RPM_BUILD_ROOT
%setup

###############################################################################
# The current directory is the one main directory of the tar
# Order of upgrade is:
#%pretrans new
#%pre new
#install new
#%post new
#%preun old
#delete old
#%postun old
#%posttrans new
%build
#autoreconf -i
%configure --disable-debug %{?checkdoc} \
    --with-tagfiles-prefix=%{src_tagfiles_prefix} \
    --with-tagfiles-suffix=%{src_tagfiles_suffix} \
    --with-htmldirs-prefix=%{target_htmldirs_prefix} \
    --with-htmldirs-suffix=%{target_htmldirs_suffix}

%{__make}

%install
%{__make} install DESTDIR=$RPM_BUILD_ROOT  %{?mflags_install}
mkdir -p $RPM_BUILD_ROOT/%{target_doc_dir}
cp ChangeLog $RPM_BUILD_ROOT/%{target_doc_dir}
rm -f $RPM_BUILD_ROOT/%{target_lib_dir}/lib*.la


%post

%post doc
rm -f %{target_doc_dir}/last
ls %{target_doc_dir} \
| grep -q '^[0-9]\+[0-9.]*[0-9]\+$' \
&& ln -s $( \
    ls %{target_doc_dir} | \
        grep '^[0-9]\+[0-9.]*[0-9]\+$' | \
        sort | tail -n1 \
) %{target_doc_dir}/last \
|| true

%postun

%postun doc
rm -f %{target_doc_dir}/last
ls %{target_doc_dir} \
| grep -q '^[0-9]\+[0-9.]*[0-9]\+$' \
&& ln -s $( \
    ls %{target_doc_dir} | \
        grep '^[0-9]\+[0-9.]*[0-9]\+$' | \
        sort | tail -n1 \
) %{target_doc_dir}/last \
|| true

%preun

%clean
cd /tmp
rm -rf $RPM_BUILD_ROOT/%{name}-%{version}
test "x$RPM_BUILD_ROOT" != "x" && rm -rf $RPM_BUILD_ROOT

###############################################################################
# Specify files to be placed into the package
%files
%defattr(-,root,root)
%{target_lib_dir}/lib*
%{target_python_lib_dir}/*
%exclude %{target_python_lib_dir}/bxi/*_cffi_def.py*
%exclude %{target_python_lib_dir}/bxi/util/*cffi_builder.py*
%doc
    %{target_doc_dir}/ChangeLog
#%{target_bin_dir}/*
%files devel
%{_includedir}/bxi/util/*.h
%{target_python_lib_dir}/bxi/*_cffi_def.py*
%{target_python_lib_dir}/bxi/util/*cffi_builder.py*




#%config(noreplace) %{target_conf_dir}/my.conf

# Changelog is automatically generated (see Makefile)
# The %doc macro already contain a default path (usually /usr/doc/)
# See:
# http://www.rpm.org/max-rpm/s1-rpm-inside-files-list-directives.html#S3-RPM-INSIDE-FLIST-DOC-DIRECTIVE for details
# %doc ChangeLog
# or using an explicit path:


#%{target_bin_dir}/bin1
#%{target_bin_dir}/prog2
#%{target_bin_dir}/exe3
%files doc
    %{target_doc_dir}/%{version}/
%files tests
    %{target_doc_dir}/tests/

# %changelog is automatically generated by 'make log' (see the Makefile)
##################### WARNING ####################
## Do not add anything after the following line!
##################################################
%changelog
