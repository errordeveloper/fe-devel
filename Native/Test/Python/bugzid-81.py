import fabric
F = fabric.createClient()

# create a new dgnode
dgnode1 = F.DG.createNode("myNode1")
dgnode2 = F.DG.createNode("myNode2")

# add some members
dgnode1.addMember('a','Scalar')
dgnode1.addMember('b','Scalar')
dgnode1.addMember('result','Scalar')

dgnode2.addMember('result','Scalar')

# increase the slicecount of the nodes
count = 1000000
dgnode1.resize(count)
dgnode2.resize(count)

# create dependencies between the nodes
dgnode2.setDependency(dgnode1, 'otherNode')

# create three operators, one to initiate the data, one to add, and one to mul
operatorInit = F.DG.createOperator("initiate")
operatorInit.setSourceCode(
  'operator initiate(in Size index, io Scalar a, io Scalar b) {\n'+
  '  a = 12.5 * Scalar(index);\n'+
  '  b = 1.73 * Scalar(index);\n'+
  '}\n')
operatorInit.setEntryFunctionName('initiate')
if len( operatorInit.getErrors() ) > 0:
  if len( operatorInit.getDiagnostics() ) > 0:
    print(fabric.stringify(opreatorInit.getDiagnostics()))

operatorAdd = F.DG.createOperator("add")
operatorAdd.setSourceCode(
  'operator add(io Scalar a, io Scalar b, io Scalar result) {\n'+
  '  result = a + b;\n'+
  '}\n')
operatorAdd.setEntryFunctionName('add')

if len( operatorAdd.getErrors() ) > 0:
  if len( operatorAdd.getDiagnostics() ) > 0:
    print(fabric.stringify(opreatorAdd.getDiagnostics()))

operatorMul = F.DG.createOperator("operatorMul")
operatorMul.setSourceCode(
  'operator mul(io Scalar input<>, io Scalar result, in Size index) {\n'+
  '  result = 3.0 * input[index];\n'+
  '}\n')
operatorMul.setEntryFunctionName('mul')

if len( operatorMul.getErrors() ) > 0:
  if len( operatorMul.getDiagnostics() ) > 0:
    print(fabric.stringify(opreatorMul.getDiagnostics()))

# create bindings between the nodes and the operator
binding1init = F.DG.createBinding()
binding1init.setOperator(operatorInit)
binding1init.setParameterLayout([
  'self.index',
  'self.a',
  'self.b'
])
binding1add = F.DG.createBinding()
binding1add.setOperator(operatorAdd)
binding1add.setParameterLayout([
  'self.a',
  'self.b',
  'self.result'
])
binding2 = F.DG.createBinding()
binding2.setOperator(operatorMul)
binding2.setParameterLayout([
  'otherNode.result<>',
  'self.result',
  'self.index'
])

# append the new binding to the node
dgnode1.bindings.append(binding1init)
dgnode1.bindings.append(binding1add)
if len( dgnode1.getErrors() ) > 0:
  print(fabric.stringify(dgnode1.getErrors()))
dgnode2.bindings.append(binding2)
if len( dgnode2.getErrors() ) > 0:
  print(fabric.stringify(dgnode2.getErrors()))

# evaluate the node!
dgnode2.evaluate()

# query the node's data (only for certain indices)
displayCount = 100 if count > 100 else count
indices = []
for i in range( 0, displayCount ):
  indices.append(i)

print(fabric.stringify(dgnode1.getSlicesBulkData(indices)))
print(fabric.stringify(dgnode2.getSlicesBulkData(indices)))

F.close()
