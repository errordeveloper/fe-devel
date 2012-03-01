/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

FC = require('Fabric').createClient();

console.log(
  FC.MR.createValueTransform(
    FC.MR.createConstValue('Integer', -42),
    FC.KLC.createValueTransformOperator(
      "xfo.kl", "operator xfo(io Integer value) { value *= -2; }", "xfo"
      )
    ).produce()
  );

FC.close();
