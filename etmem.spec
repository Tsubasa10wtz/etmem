%global debug_package %{nil}

Name:	       etmem
Version:       1.0
Release:       1
Summary:       etmem 
License:       Mulan PSL v2
Source0:       etmem-%{version}.tar.gz

#Dependency
BuildRequires: cmake
BuildRequires: libboundscheck
Requires: libboundscheck

%description
etmem module

#Build sections
%prep
%autosetup -n etmem-%{version} -p1

%build
mkdir -p build
cd build
cmake .. 
make

%install
mkdir -p $RPM_BUILD_ROOT%{_bindir}
install -d $RPM_BUILD_ROOT%{_sysconfdir}/etmem/

install -m 0700 build/bin/etmem $RPM_BUILD_ROOT%{_bindir}
install -m 0700 build/bin/etmemd $RPM_BUILD_ROOT%{_bindir}
install -m 0600 conf/example_conf.yaml $RPM_BUILD_ROOT%{_sysconfdir}/etmem/

%files
%defattr(-,root,root,0750)
%{_bindir}/etmem
%{_bindir}/etmemd
%dir %{_sysconfdir}/etmem
%{_sysconfdir}/etmem/example_conf.yaml

%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig

%changelog
* Thu Mar 18 2021 louhongxiang <louhongxiang@huawei.com>
- Package init
