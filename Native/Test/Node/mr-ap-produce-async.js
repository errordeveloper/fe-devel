/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

var fabricClient = require('Fabric').createClient();

var ap = fabricClient.MR.createArrayGenerator(
  fabricClient.MR.createConstValue('Size', 16),
  fabricClient.KLC.createArrayGeneratorOperator(
    'foo.kl',
    'operator foo(io Scalar x, Size index) { x = 3.141 * Scalar(index); }',
    'foo'
    )
  );

ap.produceAsync(function (result) {
  console.log(result);
  ap.produceAsync(5, function (result) {
    console.log(result);
    ap.produceAsync(4, 8, function (result) {
      console.log(result);
      fabricClient.close();
      });
    });
  });
