Changes in EPM
==============

Changes in EPM 5.0.1
--------------------

- Added functionality for `templates` and `changelog` files for Debian `%literal()`
  support.
- Updated documentation to include description of new functionality - Section 4

Changes in EPM 5.0.0
--------------------

- Enabling the GUI correctly checks for FLTK
- The Apache OpenOffice specific changes are no longer configure-time
  (buildtime) enabled, but run time, via the --aoo-mode CLI option.
- Relicensed to the ALv2 (via copyright holder)

Changes in EPM 4.5.2
--------------------

- Use `fakeroot` if available when building DEB packages to ensure
  correct file permissions
- Support `rpmbuild` for RHEL7 and similar platforms
- Brought back support for platforms deprecated in 4.5.0

Changes in EPM 4.5.1
--------------------

- Fixed Debian dependencies for min/max version (Issue #64)
- Fixed a typo in the epm.list file (Issue #78)
- Fixed macro usage in the setup.types man page (Issue #81)
- Fixed @INSTALL@ macro in makefile (Issue #84)
- Fixed Debian init script support (Issue #85)


Changes in EPM 4.5
------------------

- "make install" failed due to the README filename changing (Issue #59)
- The `mkepmlist` utility did not correctly handle filenames containing the
  `$` character (Issue #62)
- The configure script incorrectly substituted "NONE" for the installation
  prefix (Issue #67)
- Fixed some over-zealous permissions on temporary build directories (Issue #71)
- Fixed the mantohtml helper program - was depending on private CUPS headers for
  some reason (Issue #72)
- Fixed some build system issues (Issue #75, Issue #76)
- Documentation updates (Issue #74, Issue #77)
- Cleaned up old sprintf and strcpy usage in many places.

Changes in EPM 4.4.2
--------------------

- Support Apache OpenOffice patches (at configure time)

Changes in EPM 4.4.1
--------------------

- "make install" failed due to the README filename changing (Issue #59)
- The `mkepmlist` utility did not correctly handle filenames containing the
  `$` character (Issue #62)
- The configure script incorrectly substituted "NONE" for the installation
  prefix (Issue #67)
- Cleaned up old sprintf and strcpy usage in many places.


Changes in EPM 4.4
------------------

- The default prefix is now the usual `/usr/local` (Issue #45)
- Really fix 64-bit Intel packages on Debian-based OS's (Issue #48)
- Fixed a build issue on Solaris 11 (Issue #50)
- Fixed a bug in temporary file cleanup when symlinks are used (Issue #51)
- Added DESTDIR support to makefiles (Issue #55)
- Fixed RPM support on AIX (Issue #56)
- Reverted the hard links optimization from EPM 4.2 since it is causing
  problems with the latest version of RPM (Issue #57)
- Packages on macOS now use "macos" as the operating system name for
  consistency.


Changes in EPM 4.3
------------------

- Now use pkgbuild on newer versions of macOS, and added support for signed
  packages (Bug #497)
- Fixed some file handling issues when creating RPM packages (Bug #523)
- EPM now maps the x86_64 architecture to amd64 when creating Debian packages
  (Bug #295)
- %format stopped working in EPM 4.2 (Bug #296)
- %literal(spec) did not insert the literal content in the correct location
  (Bug #302)
- Fixed some incorrect string handling (Bug #290)
- Fixed a compatibility issue with RPM 4.8 (Bug #292)
- Fixed a build dependency problem (Bug #291)
- The EPM makefile now uses CPPFLAGS from the configure script (Bug #300)
- Updated the standard path used by portable package scripts to include
  /usr/gnu/bin for Solaris (Bug #301)
- Added support for %literal(control) in Debian packages (Bug #297)


Changes in EPM 4.2
------------------

- EPM now supports a %arch conditional directive (STR #27)
- EPM now uses hard links whenever possible instead of copying files for
  distribution (STR #21)
- EPM no longer puts files in /export in the root file set for AIX packages
  (STR #15)
- EPM did not work with newer versions of RPM (STR #23, STR #25)
- EPM did not clean up temporary files from Solaris packages (STR #20)
- Building Solaris gzip'd packages failed if the pkg.gz file already existed
  (STR #16)
- Fixed handling of %preremove and %postremove for AIX packages (STR #22)
- Fixed directory permissions in HP-UX packages (STR #24)
- Removed unnecessary quoting of "!" in filenames (STR #26)
- Added support for signed RPM packages (STR #19)
- Added support for inclusion of format-specific packaging files and directives
  via a %literal directive (STR #5)
- *BSD init scripts were not installed properly.
- EPM now displays a warning message when a variable is undefined (STR #10)
- *BSD dependencies on versioned packages are now specified correctly (STR #4)
- EPM now uses /usr/sbin/pkg_create on FreeBSD (STR #2)
- FreeBSD packages are now created with a .tbz extension (STR #1)
- FreeBSD packages incorrectly assumed that chown was installed in /bin (STR #3)
- Added support for an "lsb" package format which uses RPM with the LSB
  dependencies (STR #7)
- The configure script now supports a --with-archflags and no longer
  automatically builds universal binaries on macOS.
- The epm program now automatically detects when the setup GUI is not available,
  displays a warning message, and then creates a non-GUI package.
- RPM packages did not map %replaces to Obsoletes:


Changes in EPM 4.1
------------------

- macOS portable packages did not create a correct Uninstall application.
- The temporary package files for portable packages are now removed after
  creation of the .tar.gz file unless the -k (keep files) option is used.
- The RPM summary string for subpackages did not contain the first line of the
  package description as for other package formats.
- The setup and uninst GUIs now support installing and removing RPM packages.
- The setup GUI now confirms acceptance of all licenses prior to installing the
  first package.
- Subpackages are no longer automatically dependent on the main package.
- Multi-line descriptions were not embedded properly into portable package
  install/patch/remove scripts.
- Updated the setup and uninstall GUIs for a nicer look-n-feel.
- macOS portable packages now show the proper name, version, and copyright
  for the packaged software instead of the EPM version and copyright.
- Fixed a problem with creation of macOS metapackages with the latest Xcode.
- EPM now removes the individual .rpm and .deb files when
  creating a package with subpackages unless the -k option (keep files) is used.
- EPM now only warns about package names containing characters other than
  letters and numbers.
- EPM now generates disk images as well as a .tar.gz file when creating portable
  packages on macOS.


Changes in EPM 4.0
------------------

- New subpackage support for creating multiple dependent packages or a combined
  package with selectable subpackages, depending on the package format.
- Added support for compressing the package files in portable packages which
  reduces disk space requirements on platforms that provide gzip.
- Added support for custom platform names via the new `-m name` option.
- Added support for non-numeric %release values.
- Added new `--depend` option to list all of the source files that a package
  depends on.
- The setup GUI now sets the `EPM_INSTALL_TYPE` environment variable to the
  value of the selected TYPE line in the `setup.types` file.
- Fixed NetBSD and OpenBSD packaging support by no longer using FreeBSD-specific
  extensions to pkg_create on those variants.
- Fixed PowerPC platform support for RPM and Debian packages.
- Many fixes to AIX package support.
- Tru64 packages with init scripts now work when installing for the first time.
- RPM file dependencies should now work properly.
- Portable product names containing spaces will now display properly.
