/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

FABRIC = require('Fabric').createClient();
require( "./include/unitTestUtils.js" );
require( "../../../Web/RT/Math.js" );
Math.verboseLogFunction = console.log;

FABRIC.UnitTestUtils.loadType( 'Vec3' );
FABRIC.UnitTestUtils.loadType( 'Mat33' );
FABRIC.UnitTestUtils.loadType( 'Vec4' );
FABRIC.UnitTestUtils.appendToKLCode('Mat44', "\nfunction Vec4 Mat44.multiplyVector( in Vec4 other ) {\n  return this * other;\n}\n\n");
FABRIC.UnitTestUtils.appendToKLCode('Mat44', "\nfunction Vec3 Mat44.multiplyVector( in Vec3 other ) {\n  return this * other;\n}\n\n");
FABRIC.UnitTestUtils.appendKLOpAdaptors('Mat44', [ '*', '*=' ] );
FABRIC.UnitTestUtils.loadType( 'Mat44' );
FABRIC.UnitTestUtils.defineInPlaceOpAdaptors('Mat44', [ '*=' ] );
FABRIC.UnitTestUtils.appendKLOpAdaptors('Euler', []);
FABRIC.UnitTestUtils.loadType( 'Euler' );
FABRIC.UnitTestUtils.loadType( 'Quat' );

FABRIC.UnitTestUtils.appendKLOpAdaptors('Xfo', [ '*', '*=' ] );
FABRIC.UnitTestUtils.loadType( 'Xfo' );
FABRIC.UnitTestUtils.defineInPlaceOpAdaptors('Xfo', [ '*=' ] );

FABRIC.UnitTestUtils.runTests( 'Xfo', [['Xfo','xfo1'], ['Xfo','xfo2'], ['Xfo','res'], ['RotationOrder','ro1'], ['Euler','e1'], ['Quat','q1'], ['Mat44','m1'], ['Mat44','m2'], ['Vec3','v1'], ['Vec3','v2']] );

FABRIC.close();
