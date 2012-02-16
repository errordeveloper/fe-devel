import fabric
F = fabric.createClient()

dgnode1 = F.DG.createNode("myNode1")
dgnode1.addMember('a','Size[]')
dgnode1.resize(32)

operatorInit = F.DG.createOperator("initiate")
operatorInit.setSourceCode(
  'operator initiate(in Size index, io Size a[]) {\n'+
  '  report "Setting index " + index;\n'+
  '  a.push(index);\n'+
  '}\n')
operatorInit.setEntryFunctionName('initiate')
operatorInit.setMainThreadOnly(True)

if len( operatorInit.getErrors() ) > 0:
  if len( operatorInit.getDiagnostics() ) > 0:
    print('operatorInit1: '+fabric.stringify(operatorInit.getDiagnostics()))

bindingInit = F.DG.createBinding()
bindingInit.setOperator(operatorInit)
bindingInit.setParameterLayout([
  'self.index',
  'self.a'
])

operatorInit2 = F.DG.createOperator("initiate2")
operatorInit2.setSourceCode(
  'operator initiate2(in Container container, io Size a<>[]) {\n'+
  '  for (Size i=0; i<container.size; ++i)\n'+
  '    a[i].push(Size(2*i));\n'+
  '}\n')
operatorInit2.setEntryFunctionName('initiate2')
operatorInit2.setMainThreadOnly(True)

if len( operatorInit2.getErrors() ) > 0:
  if len( operatorInit2.getDiagnostics() ) > 0:
    print('operatorInit2: '+fabric.stringify(operatorInit2.getDiagnostics()))

bindingInit2 = F.DG.createBinding()
bindingInit2.setOperator(operatorInit2)
bindingInit2.setParameterLayout([
  'self',
  'self.a<>'
])

dgnode1.bindings.append(bindingInit)
dgnode1.bindings.append(bindingInit2)

reportOp = F.DG.createOperator('reportOp')
reportOp.setSourceCode("\
operator reportValues(io Size a<>[]) {\n\
report a;\n\
}\n\
")
reportOp.setEntryFunctionName('reportValues')

reportBindings = F.DG.createBinding()
reportBindings.setOperator(reportOp)
reportBindings.setParameterLayout([
  'node.a<>'
])

eh = F.DG.createEventHandler("eh")
eh.setScope('node', dgnode1)
eh.preDescendBindings.append(reportBindings)

e = F.DG.createEvent('e')
e.appendEventHandler(eh)
e.fire()

F.close()
