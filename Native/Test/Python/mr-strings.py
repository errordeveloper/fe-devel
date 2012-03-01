#
#  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
#

import fabric

client = fabric.createClient()

ag = client.MR.createArrayGenerator(
  client.MR.createConstValue("Size", 100),
  client.KLC.createArrayGeneratorOperator(
    "gen.kl",
    "operator gen(io String output, Size index) { output = String(index+1); }",
    "gen"
    )
  )

print(
  len( client.MR.createReduce(ag,
    client.KLC.createReduceOperator(
      "reduce.kl",
      "operator reduce(String input, io String output) { if (output.length > 0) output += '-'; output += input; }",
      "reduce"
      )
    ).produce() )
  )

client.close()
