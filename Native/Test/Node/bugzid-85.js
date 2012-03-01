/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

F = require('Fabric').createClient();

  var dgnode1 = F.DG.createNode("myNode1");
  dgnode1.addMember('a','Size[]');
  dgnode1.resize(32);
  
  var operatorInit = F.DG.createOperator("initiate");
  operatorInit.setSourceCode(
    'operator initiate(in Size index, io Size a[]) {\n'+
    '  report "Setting index " + index;\n'+
    '  a.push(index);\n'+
    '}\n');
  operatorInit.setEntryFunctionName('initiate');
  operatorInit.setMainThreadOnly(true);
  if (operatorInit.getErrors().length > 0) {
    if (operatorInit.getDiagnostics().length > 0)
      console.log(JSON.stringify(operatorInit.getDiagnostics()));
  }

  var bindingInit = F.DG.createBinding();
  bindingInit.setOperator(operatorInit);
  bindingInit.setParameterLayout([
    'self.index',
    'self.a'
  ]);

  var operatorInit2 = F.DG.createOperator("initiate2");
  operatorInit2.setSourceCode(
    'operator initiate2(in Container container, io Size a<>[]) {\n'+
    '  for (Size i=0; i<container.size; ++i)\n'+
    '    a[i].push(Size(2*i));\n'+
    '}\n');
  operatorInit2.setEntryFunctionName('initiate2');
  operatorInit2.setMainThreadOnly(true);
  if (operatorInit2.getErrors().length > 0) {
    if (operatorInit2.getDiagnostics().length > 0)
      console.log(JSON.stringify(operatorInit2.getDiagnostics()));
  }

  var bindingInit2 = F.DG.createBinding();
  bindingInit2.setOperator(operatorInit2);
  bindingInit2.setParameterLayout([
    'self',
    'self.a<>'
  ]);

  dgnode1.bindings.append(bindingInit);
  dgnode1.bindings.append(bindingInit2);

  var reportOp = F.DG.createOperator('reportOp');
  reportOp.setSourceCode("\
operator reportValues(io Size a<>[]) {\n\
  report a;\n\
}\n\
");
  reportOp.setEntryFunctionName('reportValues');

  var reportBindings = F.DG.createBinding();
  reportBindings.setOperator(reportOp);
  reportBindings.setParameterLayout([
    'node.a<>'
  ]);
  
  var eh = F.DG.createEventHandler("eh");
  eh.setScope('node', dgnode1);
  eh.preDescendBindings.append(reportBindings);
  
  var e = F.DG.createEvent('e');
  e.appendEventHandler(eh);
  e.fire();

F.close();
