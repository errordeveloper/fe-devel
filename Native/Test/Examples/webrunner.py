import subprocess
import httplib
import sys
import os
import time

# run a web script, connect to it on port 1337 and print the output

if len( sys.argv ) < 3:
  print 'invalid command-line args'
  sys.exit(1)

program = sys.argv[1]
script = sys.argv[2]

# to have access to any local .kl files
os.chdir( os.path.dirname( script ) )

try:
  # ignore stdout/stderr
  p = subprocess.Popen(
    [ program, os.path.basename( script ) ],
    stdout = subprocess.PIPE,
    stderr = subprocess.PIPE
  )

  http = httplib.HTTPConnection( 'localhost', 1337 )
  failures = 0
  while True:
    failed = False
    try:
      http.connect()
    except Exception:
      failed = True

    if failed and failures < 5:
      failures += 1
      time.sleep( 1 )
    elif failed:
      raise Exception( 'too many connection failures' )
    else:
      break

  http.request( 'GET', '/' )
  r = http.getresponse()
  print r.read()

except Exception as e:
  print e

finally:
  p.kill()

