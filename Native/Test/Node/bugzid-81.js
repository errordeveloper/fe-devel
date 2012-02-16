F = require('Fabric').createClient();

  // create a new dgnode
  var dgnode1 = F.DG.createNode("myNode1");
  var dgnode2 = F.DG.createNode("myNode2");
  
  // add some members
  dgnode1.addMember('a','Scalar');
  dgnode1.addMember('b','Scalar');
  dgnode1.addMember('result','Scalar');
  
  dgnode2.addMember('result','Scalar');
  
  // increase the slicecount of the nodes
  var count = 1000000;
  dgnode1.resize(count);
  dgnode2.resize(count);
  
  // create dependencies between the nodes
  dgnode2.setDependency(dgnode1, 'otherNode');
  
  // create three operators, one to initiate the data, one to add, and one to mul
  var operatorInit = F.DG.createOperator("initiate");
  operatorInit.setSourceCode(
    'operator initiate(in Size index, io Scalar a, io Scalar b) {\n'+
    '  a = 12.5 * Scalar(index);\n'+
    '  b = 1.73 * Scalar(index);\n'+
    '}\n');
  operatorInit.setEntryFunctionName('initiate');
  if (operatorInit.getErrors().length > 0) {
    if (operatorInit.getDiagnostics().length > 0)
      console.log(JSON.stringify(opreatorInit.getDiagnostics()));
  }
  
  var operatorAdd = F.DG.createOperator("add");
  operatorAdd.setSourceCode(
    'operator add(io Scalar a, io Scalar b, io Scalar result) {\n'+
    '  result = a + b;\n'+
    '}\n');
  operatorAdd.setEntryFunctionName('add');
  if (operatorAdd.getErrors().length > 0) {
    if (operatorAdd.getDiagnostics().length > 0)
      console.log(JSON.stringify(opreatorInit.getDiagnostics()));
  }
  
  var operatorMul = F.DG.createOperator("operatorMul");
  operatorMul.setSourceCode(
    'operator mul(io Scalar input<>, io Scalar result, in Size index) {\n'+
    '  result = 3.0 * input[index];\n'+
    '}\n');
  operatorMul.setEntryFunctionName('mul');
  if (operatorMul.getErrors().length > 0) {
    if (operatorMul.getDiagnostics().length > 0)
      console.log(JSON.stringify(opreatorInit.getDiagnostics()));
  }
  
  // create bindings between the nodes and the operator
  var binding1init = F.DG.createBinding();
  binding1init.setOperator(operatorInit);
  binding1init.setParameterLayout([
    'self.index',
    'self.a',
    'self.b'
  ]);
  var binding1add = F.DG.createBinding();
  binding1add.setOperator(operatorAdd);
  binding1add.setParameterLayout([
    'self.a',
    'self.b',
    'self.result'
  ]);
  var binding2 = F.DG.createBinding();
  binding2.setOperator(operatorMul);
  binding2.setParameterLayout([
    'otherNode.result<>',
    'self.result',
    'self.index'
  ]);
  
  // append the new binding to the node
  dgnode1.bindings.append(binding1init);
  dgnode1.bindings.append(binding1add);
  if (dgnode1.getErrors().length > 0)
    console.log(JSON.stringify(dgnode1.getErrors()));
  dgnode2.bindings.append(binding2);
  if (dgnode2.getErrors().length > 0)
    console.log(JSON.stringify(dgnode2.getErrors()));
  
  // evaluate the node!
  dgnode2.evaluate();
  
  // query the node's data (only for certain indices)
  var displayCount = count > 100 ? 100 : count;
  var indices = [];
  for(var i=0;i<displayCount;i++)
    indices.push(i);

  console.log(JSON.stringify(dgnode1.getSlicesBulkData(indices)));
  console.log(JSON.stringify(dgnode2.getSlicesBulkData(indices)));

F.close();
