Summary: Fault tolerant search utilities
Name: whichman
Version: 2.3
Release: 1
Copyright: GPL
Group: Utilities/File
Source: http://main.linuxfocus.org/~guido/whichman-2.3.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-root

%description
ftff, ftwhich and whichman are fault tolerant search utilities.
whichman allows to search for man pages that match approximately the specified
search key. ftff is a fault tolerant file find utility and ftwhich
is a fault tolerant version for the 'which' command.
The error tolerant approximate string match is based on the Levenshtein
Distance between two strings. This is a measure for the number of
replacements, insertions and deletions that are necessary to transform
string A into string B.

%prep
%setup

%build
make 

%install
rm -rf $RPM_BUILD_ROOT
make instroot=$RPM_BUILD_ROOT MANDIR=%{_mandir} install

%files
%doc README 
%{_mandir}/*/whichman.*
%{_mandir}/*/ftff.*
%{_mandir}/*/ftwhich.*
/usr/bin/whichman
/usr/bin/ftff
/usr/bin/ftwhich

%changelog
* Sat Jan 4 2003 Guido Socher <guido@linuxfocus.org>
first rpm version with changelog

