#
#  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
#

import fabric

FABRIC = fabric.createClient()

parent = FABRIC.DependencyGraph.createNode( "Parent" )
child = FABRIC.DependencyGraph.createNode( "Child" )

try:
  child.setDependency( parent, "" )
except Exception:
  print( "parentName must be non-empty" )

try:
  child.setDependency( parent, "self" )
except Exception:
  print( "parentName must not be 'self'" )

# works
child.setDependency( parent, "parent" )
# works again with same name
child.setDependency( parent, "parent" )
# works same node different name
child.setDependency( parent, "parentAgain" )

if child.getDependency('parent') == parent:
  print( "child.getDependency('parent').eq(parent)" )

if child.getDependency('parentAgain') == parent:
  print( "child.getDependency('parentAgain').eq(parent)" )

try:
  child.getDependency("children")
except Exception:
  print( "child.getDependency('children') fails" )

try:
  parent.getDependency("parent")
except Exception:
  print( "parent.getDependency('parent') fails" )

try:
  parent.getDependency("children")
except Exception:
  print( "parent.getDependency('children') fails" )

if child.getDependencies()[ 'parent' ] == parent:
  print( "child.getDependencies().parent.eq(parent)" )

if child.getDependencies()[ 'parentAgain' ] == parent:
  print( "child.getDependencies().parentAgain.eq(parent)" )

FABRIC.close()
