#
# spec file for package tcl-tidy
#

%{!?directory:%define directory /usr}
%define packagename tidy

Name:           tcl-tidy
Version:        0.3
Release:        0
Summary:        Tcl bindings for libtidy
License:        MIT
Group:          Development/Libraries/Tcl
Url:            https://github.com/ray2501/tcl-tidy
Source:         %{name}-%{version}.tar.gz
BuildRequires:  autoconf
BuildRequires:  make
BuildRequires:  gcc
BuildRequires:  tcl-devel >= 8.6
BuildRequires:  libtidy-devel
BuildRoot:      %{_tmppath}/%{name}-%{version}-build

%description
This package is Tcl bindings for libtidy.

%prep
%setup -q -n %{name}-%{version}

%build
./configure \
	--prefix=%{directory} \
	--exec-prefix=%{directory} \
	--libdir=%{directory}/%{_lib}
make

%install
make DESTDIR=%{buildroot} pkglibdir=%{tcl_archdir}/%{packagename}%{version} install

%clean
rm -rf %buildroot

%files
%defattr(-,root,root)
%doc README.md LICENSE
%{tcl_archdir}

%changelog

