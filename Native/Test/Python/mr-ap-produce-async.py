import fabric

fabricClient = fabric.createClient()

ap = fabricClient.MR.createArrayGenerator(
  fabricClient.MR.createConstValue('Size', 16),
  fabricClient.KLC.createArrayGeneratorOperator(
    'foo.kl',
    'operator foo(io Scalar x, Size index) { x = 3.141 * Scalar(index); }',
    'foo'
    )
  )

def callback1( result ):
  def callback2( result ):
    def callback3( result ):
      print( 'callback3: ' + str(result) )
      fabricClient.close()
    print( 'callback2: ' + str(result) )
    ap.produceAsync(  4, 8, callback3 )
  print( 'callback1: ' + str(result) )
  ap.produceAsync( 5, callback2 )

ap.produceAsync( callback1 )

