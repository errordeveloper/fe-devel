/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

FABRIC = require('Fabric').createClient();

var parent = FABRIC.DependencyGraph.createNode( "Parent" );
var child = FABRIC.DependencyGraph.createNode( "Child" );

try {
  child.setDependency( parent, "" );
}
catch (e) {
  console.log( "parentName must be non-empty" );
}

try {
  child.setDependency( parent, "self" );
}
catch (e) {
  console.log( "parentName must not be 'self'" );
}

// works
child.setDependency( parent, "parent" );
// works again with same name
child.setDependency( parent, "parent" );
// works same node different name
child.setDependency( parent, "parentAgain" );

if ( child.getDependency('parent') === parent ) {
  console.log( "child.getDependency('parent').eq(parent)" );
}

if ( child.getDependency('parentAgain') === parent ) {
  console.log( "child.getDependency('parentAgain').eq(parent)" );
}

try {
  child.getDependency("children");
}
catch (e) {
  console.log( "child.getDependency('children') fails" );
}

try {
  parent.getDependency("parent");
}
catch (e) {
  console.log( "parent.getDependency('parent') fails" );
}

try {
  parent.getDependency("children");
}
catch (e) {
  console.log( "parent.getDependency('children') fails" );
}

if ( child.getDependencies().parent === parent ) {
  console.log( "child.getDependencies().parent.eq(parent)" );
}

if ( child.getDependencies().parentAgain === parent ) {
  console.log( "child.getDependencies().parentAgain.eq(parent)" );
}

FABRIC.close();
