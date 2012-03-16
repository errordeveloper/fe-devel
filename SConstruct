#
# Copyright 2010-2011 Fabric Engine Inc. All rights reserved.
#

# [pzion 20110726] !!!!!!!!!!!!!!!!!!!!
# When you change the version number,
# don't forget to consider the expiry
# and cache generation!!
# !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
FABRIC_NAME='Fabric Engine'
FABRIC_COMPANY_NAME='Fabric Engine Inc.'
FABRIC_DESC='High Performance Computing for the Web'
FABRIC_COPYRIGHT_YEARS='2010-2012'
FABRIC_URL="http://fabric-engine.com"
FABRIC_VERSION_MAJ='1'
FABRIC_VERSION_MIN='0'
FABRIC_VERSION_REV='22'
FABRIC_VERSION_SUFFIX='-release'
FABRIC_CACHE_GENERATION='19'
# !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

import os, platform, posixpath, glob, sys

try:
  buildOS = os.environ['FABRIC_BUILD_OS']
  Export( 'buildOS' );
  buildArch = os.environ['FABRIC_BUILD_ARCH']
  Export( 'buildArch' )
  buildType = os.environ['FABRIC_BUILD_TYPE']
  Export( 'buildType' )
except:
  print "Must source fabric-build-env.sh first"
  sys.exit(1)

CacheDir( '.sconscache' )

SetOption('num_jobs', int(os.environ.get('FABRIC_BUILD_JOBS', 1)))
print "Running with -j" + str(GetOption('num_jobs'))

FABRIC_COPYRIGHT='Copyright ' + FABRIC_COPYRIGHT_YEARS + ' ' + FABRIC_COMPANY_NAME + '  All rights reserved.'
FABRIC_BASE_VERSION = FABRIC_VERSION_MAJ + '.' + FABRIC_VERSION_MIN + '.' + FABRIC_VERSION_REV
FABRIC_VERSION = FABRIC_BASE_VERSION + FABRIC_VERSION_SUFFIX

env = Environment(
  ENV = { 'PATH': os.environ['PATH'] },
  FABRIC_NAME = FABRIC_NAME,
  FABRIC_COMPANY_NAME = FABRIC_COMPANY_NAME,
  FABRIC_DESC = FABRIC_DESC,
  FABRIC_COPYRIGHT_YEARS = FABRIC_COPYRIGHT_YEARS,
  FABRIC_URL = FABRIC_URL,
  FABRIC_VERSION_MAJ = FABRIC_VERSION_MAJ,
  FABRIC_VERSION_MIN = FABRIC_VERSION_MIN,
  FABRIC_VERSION_REV = FABRIC_VERSION_REV,
  FABRIC_VERSION_SUFFIX = FABRIC_VERSION_SUFFIX,
  FABRIC_CACHE_GENERATION = FABRIC_CACHE_GENERATION,
  FABRIC_COPYRIGHT = FABRIC_COPYRIGHT,
  FABRIC_BASE_VERSION = FABRIC_BASE_VERSION,
  FABRIC_VERSION = FABRIC_VERSION,
  FABRIC_BUILD_OS=buildOS,
  FABRIC_BUILD_ARCH=buildArch,
  FABRIC_BUILD_TYPE=buildType,
  # TARGET_ARCH must be set when the Environment() is created in order
  # to pull in correct x86 vs x64 paths on Windows
  TARGET_ARCH = buildArch
  )

env.Append(BUILDERS = {
  'SubstBuildVars': Builder( action = "\
    sed \
      -e 's/{{FABRIC_COMPANY_NAME}}/"+FABRIC_COMPANY_NAME+"/g' \
      -e 's/{{FABRIC_NAME}}/"+FABRIC_NAME+"/g' \
      -e 's/{{FABRIC_DESC}}/"+FABRIC_DESC+"/g' \
      -e 's/{{FABRIC_COPYRIGHT_YEARS}}/"+FABRIC_COPYRIGHT_YEARS+"/g' \
      -e 's/{{FABRIC_COPYRIGHT}}/"+FABRIC_COPYRIGHT+"/g' \
      -e 's,{{FABRIC_URL}},"+FABRIC_URL+",g' \
      -e 's/{{FABRIC_VERSION_MAJ}}/"+FABRIC_VERSION_MAJ+"/g' \
      -e 's/{{FABRIC_VERSION_MIN}}/"+FABRIC_VERSION_MIN+"/g' \
      -e 's/{{FABRIC_VERSION_REV}}/"+FABRIC_VERSION_REV+"/g' \
      -e 's/{{FABRIC_VERSION_SUFFIX}}/"+FABRIC_VERSION_SUFFIX+"/g' \
      -e 's/{{FABRIC_VERSION}}/"+FABRIC_VERSION+"/g' \
      -e 's/{{FABRIC_CACHE_GENERATION}}/"+FABRIC_CACHE_GENERATION+"/g' \
      -e 's/{{FABRIC_BASE_VERSION}}/"+FABRIC_BASE_VERSION+"/g' \
      -e 's/{{FABRIC_VERSION}}/"+FABRIC_VERSION+"/g' \
    <$SOURCE >$TARGET \
  " )
  })

buildDir = '#build'
Export('buildDir')
distDir = env.Dir('#dist')
Export('distDir')

Export( 'env' )

SConscript( dirs=['SConsUtils'], variant_dir=os.path.join( buildDir, 'SConsUtils' ) )

nativeTargets = [
  ['kl', "The 'kl' command-line tool"],
  ['parseJSON', "The parseJSON command-line tool"],
  ['cliTools', 'All command-line tools'],
  ['node', 'The Node.js module command-line client'],
  ['python', 'The Python module command-line client'],
  ['cliClients', 'All command-line clients'],
  ['cli', 'All command-line clients and tools'],
  ['npapi', 'The NPAPI plugin browser client'],
  ['exts', 'All the extensions'],
  ['all', 'All clients and tools (command-line and browser)'],
  ['dist', 'Build packages for distribution'],
  ['upload', 'Upload distribution packages to dist.fabric-engine.com']
  ]
for nativeTarget in nativeTargets:
  if nativeTarget[0] in COMMAND_LINE_TARGETS:
    SConscript( dirs=['Native'] )
    break

docsTargets = [
  ['klpgPDF', 'KLPG in PDF form'],
  ['iopgPDF', 'IOPG in PDF form'],
  ['guides', 'All guides'],
  ['uploadDocs', 'Upload built documentation to dist.fabric-engine.com']
  ]
for docsTarget in docsTargets:
  if docsTarget[0] in COMMAND_LINE_TARGETS:
    SConscript( dirs=['Documentation'], variant_dir=os.path.join( buildDir, 'Documentation' ) )
    break
targetGroups = [
  ["Native Targets", nativeTargets],
  ["Documentation Targets", docsTargets]
  ]

options = [
  ['VERBOSE=1', "Show commands that are executed"]
  ]

def displayHelp(target, source, env):
  print """
Fabric Build System
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

USAGE: scons [<option>...] <target>...

<target> can be among the following:
"""
  for targetGroup in targetGroups:
    print "  %s:" % targetGroup[0]
    print
    for target in targetGroup[1]:
      print "  %12s  %s" % (target[0], target[1])
    print
  print "<option> can be among the following:"
  print
  for option in options:
    print "  %12s  %s" % (option[0], option[1])
  print
  print "For more information refer to http://fabric-engine.com/"
  print
Default(
  Alias('help', Command(['phony'], [], displayHelp))
  )
