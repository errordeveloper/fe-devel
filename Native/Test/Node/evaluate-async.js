var fabricModule = require('Fabric');

var testCount = 64;
var completedTestCount = 0;
for (var testIndex=0; testIndex<testCount;) {
  (function (fabricClient) {
    var o = fabricClient.DG.createOperator("o"+testIndex);
    o.setSourceCode("inline", "operator entry(in Size index, io Scalar result) { for (Size i=0; i<4096; ++i) result = log(1+index+i); }");
    o.setEntryFunctionName("entry");

    var b = fabricClient.DG.createBinding("b"+testIndex);
    b.setOperator(o);
    b.setParameterLayout(["self.index","self.a"]);

    var n = fabricClient.DG.createNode("n"+testIndex);
    n.addMember("a", "Scalar");
    n.resize(10000);
    n.bindings.append(b);
    n.evaluateAsync(function () {
      console.error("completed " + (++completedTestCount));
      fabricClient.close();
    });
  })(fabricModule.createClient());
  console.error("queued "+(++testIndex));
}
