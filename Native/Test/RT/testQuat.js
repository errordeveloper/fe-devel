FABRIC = require('Fabric').createClient();
require( "./include/unitTestUtils.js" );
require( "../../../Web/RT/Math.js" );
Math.verboseLogFunction = console.log;
FABRIC.UnitTestUtils.loadType( 'Vec3' );
FABRIC.UnitTestUtils.appendToKLCode('Mat33', "\nfunction Vec3 Mat33.multiplyVector( in Vec3 other ) {\n  return this * other;\n}\n\n");
FABRIC.UnitTestUtils.loadType( 'Mat33' );
FABRIC.UnitTestUtils.appendKLOpAdaptors('Euler', []);
FABRIC.UnitTestUtils.loadType( 'Euler' );

FABRIC.UnitTestUtils.appendKLOpAdaptors('Quat', [ '+', '+=', '-', '-=', '*', '*=', ['*','Scalar'], ['*=','Scalar'], '/', '/=', ['/','Scalar'], ['/=','Scalar'] ] );
FABRIC.UnitTestUtils.loadType( 'Quat' );
FABRIC.UnitTestUtils.defineInPlaceOpAdaptors('Quat', [ '+=', '-=', '*=', [ '*=', 'Scalar' ], '/=', [ '/=', 'Scalar' ] ] );

FABRIC.UnitTestUtils.runTests( 'Quat', [['Quat','q1'], ['Quat','q2'], ['Quat','res'], ['RotationOrder','ro1'], ['Euler','e1'], ['Mat33','m1'], ['Vec3','v1'], ['Vec3','v2']] );

FABRIC.close();
