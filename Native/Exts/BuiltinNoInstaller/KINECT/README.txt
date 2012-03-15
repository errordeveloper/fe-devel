Fabric Engine Kinect Extension
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

This archive contains the Fabric Kinect extensions, which
provides the ability to interface with Kinect devices to Fabric
programs.  This extension is not installed along with the
Fabric browser plugin since it requires that both the
Microsoft Visual C++ 2010 Redistributable Package and the
Kinect for Windows SDK be installed.

Installation
------------

First, install the Microsoft Visual C++ 2010 Redistributable
Package.  As of this writing, it is available at:

  http://www.microsoft.com/download/en/details.aspx?id=5555

Next, install the Kinect for Windows SDK (last tested: v1.0 Beta 2).
As of this writing it is available at:

  http://www.microsoft.com/download/en/details.aspx?id=27876

Finally, to install the Fabric Kinect extension, just copy
the contents of the Exts subdirectory into the directory

  %APPDATA%\Fabric\Exts

where %APPDATA% is the value of the APPDATA environment
variable.  Note that you may need to create the directory
first.

Note that you will need to restart any browsers running
Fabric for the extension to be loaded.

Additional Information
----------------------

Additional information on the Kinect Extension can be found online
in the Developer Install Guide:

http://documentation.fabric-engine.com/latest/FabricEngine-DeveloperInstallGuide.html
