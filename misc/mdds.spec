Name:           mdds
Version:        0.3.1
Release:        1
Url:            http://code.google.com/p/multidimalgorithm/
License:        MIT/X11
Source:         mdds_0.3.1.tar.bz2
Group:          Development/Libraries/C and C++
Summary:        A collection of multi-dimensional data structure and indexing algorithm
BuildRoot:      %{_tmppath}/%{name}-%{version}-build
BuildRequires:  gcc-c++, libstdc++-devel, boost-devel 
Requires:       boost-devel >= 1.39

%description
This library provides a collection of multi-dimensional data structure and indexing
algorithm.  All data structures are available as C++ templates, hence this is a 
header-only library, with no shared library to link against.

Authors:
--------
    Kohei Yoshida <kyoshida@novell.com>

%package        devel
Url:            http://code.google.com/p/multidimalgorithm/
License:        MIT/X11
Group:          Development/Libraries/C and C++
Summary:        A collection of multi-dimensional data structure and indexing algorithm

%description    devel
This library provides a collection of multi-dimensional data structure and indexing
algorithms.  All data structures are available as C++ templates, hence this is a 
header-only library, with no shared library to link against.

Authors:
--------
    Kohei Yoshida <kyoshida@novell.com>

%define _docdir %{_defaultdocdir}/mdds-devel

%prep
%setup -q -n %{name}_%{version}

%build
./configure --prefix=%buildroot/usr

%check
make check

%install
make install

%clean
rm -rf %buildroot

%files devel
%defattr(-,root,root)
%{_includedir}/mdds  
%{_datadir}/mdds-devel/example  
%doc %{_docdir}/*  

%changelog
