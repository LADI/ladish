Summary: A collection of plugins useful for modular synths.
Name: omins
Version: 0.0.1
Release: 1
License: GPL
Group: Applications/Multimedia
URL: http://www.nongnu.org/om-synth/
Source0: %{name}-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-buildroot
Requires:	ladspa
BuildRequires: ladspa-devel

%description
Omins is a collection of ladspa plugins that are useful for modular synths. This includes a formant filter, envelope generators, probalistic switches, and rate converters. 

%prep
%setup -q

%build
%configure
make

%install
%{__rm} -rf %{buildroot}
%{__make} install \
	DESTDIR="%{buildroot}"


%clean
rm -rf %{buildroot}

%files
%defattr(-,root,root,-)
%{_libdir}/ladspa/*.so
%doc AUTHORS ChangeLog COPYING NEWS README


%changelog
* Tue Apr 26 2005 Loki Davison <loki@berlios.de>
- initial package, v0.0.1

