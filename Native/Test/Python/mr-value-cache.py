#
#  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
#

import fabric

client = fabric.createClient()

gen = client.MR.createValueCache(
    client.MR.createValueGenerator(
      client.KLC.createValueGeneratorOperator(
        "xfo.kl", "operator xfo(io Integer value) { report 'Running generator'; value = 121; }", "xfo"
        )
      )
    )

print("Should run generator")
print(gen.produce())

print("Should not run generator (cached)")
print(gen.produce())

gen.flush()
print("Should run generator")
print(gen.produce())

client.close()
