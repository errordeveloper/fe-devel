Fabric Engine FileSystem Extension README.txt
Copyright (C) 2010-2012 Fabric Engine Inc.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

Introduction
------------

This archive contains the Fabric FileSystem extensions, which
provides direct filesystem access to Fabric programs.  This
extension is not installed along with the Fabric browser plugin
since it creates a security risk; it should only be installed
on systems where the Fabric applications that are going to
be run are known and trusted.

Installation
------------

To install the extension, just copy the contents of the Exts
subdirectory into the following directory, creating the directory
if necessary.  The directory is platform-dependent:

Windows: %APPDATA%\Fabric\Exts
  (where %APPDATA% is the value of the APPDATA environment variable)
OS X: ~/Library/Fabric/Exts
Linux: ~/.fabric/Exts

Note that you will need to restart any browsers running 
Fabric for the extension to be loaded.

Additional Information
----------------------

Additional information on the FileSystem Extension can be found online
in the Developer Install Guide:

http://documentation.fabric-engine.com/latest/FabricEngine-DeveloperInstallGuide.html
