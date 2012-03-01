#
#  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
#

import fabric

client = fabric.createClient()

cv = client.MR.createConstValue('String', 'Hello')

vmo = client.KLC.createValueMapOperator("vm.kl", "operator vm( String input, io String output ) { output = input + ', world!'; }", "vm")

vm = client.MR.createValueMap(cv, vmo)

print(vm.produce())

client.close()
