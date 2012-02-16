import fabrictest
import json
import fabric
F = fabric.createClient()

b = F.DG.createBinding()
print(fabrictest.stringify(b.getOperator()))
print(fabrictest.stringify(b.getParameterLayout()))

o = F.DG.createOperator("op")
b.setOperator(o)
print(fabrictest.stringify(b.getOperator().getName()))

b.setParameterLayout(["self.foo"])
print(fabrictest.stringify(b.getParameterLayout()))

F.close()

