/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

FC = require('Fabric').createClient();

var gen = 
  FC.MR.createValueCache(
    FC.MR.createValueGenerator(
      FC.KLC.createValueGeneratorOperator(
        "xfo.kl", "operator xfo(io Integer value) { report 'Running generator'; value = 121; }", "xfo"
        )
      )
    );

console.log("Should run generator");
console.log(gen.produce());

console.log("Should not run generator (cached)");
console.log(gen.produce());

gen.flush();
console.log("Should run generator");
console.log(gen.produce());

FC.close();
