%define buildroot %{_tmppath}/%{name}

Name:          rivet-fcgi
Summary:       Tcl interpreter and Rivet tempalte parser for FastCGI
Version:       0.2.3
Release:       2
License:       Apache-2.0
Group:         Productivity/Networking/Web/Servers
Source:        %{name}-%{version}.tar.gz
URL:           https://github.com/ray2501/rivet-fcgi
BuildRequires: cmake
BuildRequires: make
BuildRequires: tcl-devel >= 8.6.1
BuildRequires: FastCGI-devel
Requires:      tcl >= 8.6.1
BuildRoot:     %{buildroot}

%description
This is a research and prototype project.
The idea is to embed a Tcl interpreter and
Rivet tempalte parser in a FastCGI application.

%prep
%setup -q -n %{name}-%{version}

%build
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=/usr -DINSTALL_LIB_DIR=%{_lib} \
-DRIVETLIB_DESTDIR=%{_lib}/tcl/rivetfcgi%{version} \
-DDESTDIR=%{buildroot} ..
cmake --build .

%install
cd build
make DESTDIR=%{buildroot} install

%clean
rm -rf %buildroot

%files
/usr/bin/rivet-fcgi
%{tcl_archdir}/rivetfcgi%{version}
