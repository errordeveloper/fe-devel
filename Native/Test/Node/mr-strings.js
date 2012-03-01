/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

FC = require('Fabric').createClient();

ag = FC.MR.createArrayGenerator(
  FC.MR.createConstValue("Size", 100),
  FC.KLC.createArrayGeneratorOperator(
    "gen.kl",
    "operator gen(io String output, Size index) { output = String(index+1); }",
    "gen"
    )
  );

console.log(
  FC.MR.createReduce(ag,
    FC.KLC.createReduceOperator(
      "reduce.kl",
      "operator reduce(String input, io String output) { if (output.length > 0) output += '-'; output += input; }",
      "reduce"
      )
    ).produce().length
  );

FC.close();
