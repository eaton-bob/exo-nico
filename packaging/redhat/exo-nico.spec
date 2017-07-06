#
#    exo-nico - exercice nicolas D
#
#    Copyright (C) 2014 - 2017 Eaton                                        
#                                                                           
#    This program is free software; you can redistribute it and/or modify   
#    it under the terms of the GNU General Public License as published by   
#    the Free Software Foundation; either version 2 of the License, or      
#    (at your option) any later version.                                    
#                                                                           
#    This program is distributed in the hope that it will be useful,        
#    but WITHOUT ANY WARRANTY; without even the implied warranty of         
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          
#    GNU General Public License for more details.                           
#                                                                           
#    You should have received a copy of the GNU General Public License along
#    with this program; if not, write to the Free Software Foundation, Inc.,
#    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.            
#

# To build with draft APIs, use "--with drafts" in rpmbuild for local builds or add
#   Macros:
#   %_with_drafts 1
# at the BOTTOM of the OBS prjconf
%bcond_with drafts
%if %{with drafts}
%define DRAFTS yes
%else
%define DRAFTS no
%endif
Name:           exo-nico
Version:        1.0.0
Release:        1
Summary:        exercice nicolas d
License:        GPL-2.0+
URL:            http://example.com/
Source0:        %{name}-%{version}.tar.gz
Group:          System/Libraries
# Note: ghostscript is required by graphviz which is required by
#       asciidoc. On Fedora 24 the ghostscript dependencies cannot
#       be resolved automatically. Thus add working dependency here!
BuildRequires:  ghostscript
BuildRequires:  asciidoc
BuildRequires:  automake
BuildRequires:  autoconf
BuildRequires:  libtool
BuildRequires:  pkgconfig
BuildRequires:  systemd-devel
BuildRequires:  systemd
%{?systemd_requires}
BuildRequires:  xmlto
BuildRequires:  gcc-c++
BuildRequires:  zeromq-devel
BuildRequires:  czmq-devel
BuildRequires:  malamute-devel
BuildRoot:      %{_tmppath}/%{name}-%{version}-build

%description
exo-nico exercice nicolas d.

%package -n libexo_nico0
Group:          System/Libraries
Summary:        exercice nicolas d shared library

%description -n libexo_nico0
This package contains shared library for exo-nico: exercice nicolas d

%post -n libexo_nico0 -p /sbin/ldconfig
%postun -n libexo_nico0 -p /sbin/ldconfig

%files -n libexo_nico0
%defattr(-,root,root)
%{_libdir}/libexo_nico.so.*

%package devel
Summary:        exercice nicolas d
Group:          System/Libraries
Requires:       libexo_nico0 = %{version}
Requires:       zeromq-devel
Requires:       czmq-devel
Requires:       malamute-devel

%description devel
exercice nicolas d development tools
This package contains development files for exo-nico: exercice nicolas d

%files devel
%defattr(-,root,root)
%{_includedir}/*
%{_libdir}/libexo_nico.so
%{_libdir}/pkgconfig/libexo_nico.pc
%{_mandir}/man3/*
%{_mandir}/man7/*

%prep

%setup -q

%build
sh autogen.sh
%{configure} --enable-drafts=%{DRAFTS} --with-systemd-units
make %{_smp_mflags}

%install
make install DESTDIR=%{buildroot} %{?_smp_mflags}

# remove static libraries
find %{buildroot} -name '*.a' | xargs rm -f
find %{buildroot} -name '*.la' | xargs rm -f

%files
%defattr(-,root,root)
%doc README.md
%{_bindir}/exo-nico-main
%{_mandir}/man1/exo-nico-main*
%config(noreplace) %{_sysconfdir}/exo-nico/exo-nico-main.cfg
/usr/lib/systemd/system/exo-nico-main.service
%dir %{_sysconfdir}/exo-nico
%if 0%{?suse_version} > 1315
%post
%systemd_post exo-nico-main.service
%preun
%systemd_preun exo-nico-main.service
%postun
%systemd_postun_with_restart exo-nico-main.service
%endif

%changelog
