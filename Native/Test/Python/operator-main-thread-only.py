import fabric
F = fabric.createClient()

o = F.DG.createOperator("op")
o.setSourceCode("operator entry( io Integer foo ) { report 'Hello'; }")
o.setEntryFunctionName("entry")
print(o.getMainThreadOnly())
o.setMainThreadOnly(True)
print(o.getMainThreadOnly())

b = F.DG.createBinding()
b.setOperator(o)
b.setParameterLayout(["self.foo"])

n = F.DG.createNode("foo")
n.addMember("foo", "Integer")
n.setCount(64)
n.bindings.append(b)
n.evaluate()

F.close()
