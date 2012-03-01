var fc = require('Fabric').createClient();
try {
  var op = fc.DG.createOperator('foo');
  op.setSourceCode('foo.kl', 'operator foo(Integer bar<>[]) { report bar; }');
  op.setEntryFunctionName('foo');
  var binding = fc.DG.createBinding();
  binding.setOperator(op);
  binding.setParameterLayout([
    'self.bar<>'
    ]);
  var node = fc.DG.createNode('fooNode');
  node.addMember('bar', 'Size[]');
  node.bindings.append(binding);
  console.log('op.getErrors(): ' + op.getErrors());
  console.log('node.getErrors(): ' + node.getErrors());
  node.evaluate();
}
catch (e) {
  console.error('Caught exception: ' + e);
}
fc.close();
