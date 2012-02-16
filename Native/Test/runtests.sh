tmpfilename()
{
  if [ -n "$(which md5)" ]; then
    echo "/tmp/$(dd if=/dev/urandom bs=1024 count=1 2>/dev/null | md5).out"
  elif [ -n "$(which md5sum)" ]; then
    echo "/tmp/$(dd if=/dev/urandom bs=1024 count=1 2>/dev/null | md5sum | cut -c1-32).out"
  fi
}

if [ -z "$FABRIC_BUILD_OS" -o -z "$FABRIC_BUILD_ARCH" -o -z "$FABRIC_BUILD_TYPE" ]; then
  cat >&2 <<EOF
Must source fabric-build-env.sh first
EOF
  exit 1
fi

if [ -f "valgrind.suppressions.$FABRIC_BUILD_OS" ]; then
  EXTRA_VALGRIND_SUPP="--suppressions=valgrind.suppressions.$FABRIC_BUILD_OS"
fi

if [ -n "$FABRIC_TEST_WITH_VALGRIND" ]; then
  VALGRIND_CMD="valgrind --suppressions=../valgrind.suppressions.$FABRIC_BUILD_OS --leak-check=full -q $EXTRA_VALGRIND_SUPP"
else
  VALGRIND_CMD=
fi

REPLACE=0
OS_SPEC=0
if [ "$1" = "-r" ]; then
  REPLACE=1
  shift
fi
if [ "$1" = "-s" ]; then
  REPLACE=1
  OS_SPEC=1
  shift
fi

if [ "$FABRIC_BUILD_OS" = "Windows" ]; then
  OUTPUT_FILTER="dos2unix --d2u"
else
  OUTPUT_FILTER=cat
fi

ERROR=0
for f in "$@"; do
  TMPFILE=$(tmpfilename)

  # certain tests will always fail with valgrind and can't be suppressed
  if [ -f ${f%.$TEST_SUFFIX}.novalgrind ]; then
    CMD="$TEST_CMD $f"
  else
    CMD="$VALGRIND_CMD $VALGRIND_EXTRA $TEST_CMD $f"
  fi

  $CMD 2>&1 \
    | grep -v '^\[FABRIC\] Fabric Engine version' \
    | grep -v '^\[FABRIC\] This build of Fabric' \
    | grep -v '^\[FABRIC\] .*Extension registered' \
    | grep -v '^\[FABRIC\] .*Searching extension directory' \
    | grep -v '^\[FABRIC\] .*unable to open extension directory' \
    | $OUTPUT_FILTER >$TMPFILE

  if [ $? -ne 0 ]; then
    echo "FAIL $(basename $f)"
    echo "To debug, run:"
    echo "gdb --args" $CMD
    exit 1
  fi

  if [ "$REPLACE" -eq 1 ]; then
    if [ "$OS_SPEC" -eq 1 ]; then
      OUTFILE=${f%.$TEST_SUFFIX}.$FABRIC_BUILD_OS.$FABRIC_BUILD_ARCH.out
    else
      OUTFILE=${f%.$TEST_SUFFIX}.out
    fi
    mv "$TMPFILE" "$OUTFILE"
    echo "REPL $(basename $f)";
  else
    EXPFILE=${f%.$TEST_SUFFIX}.$FABRIC_BUILD_OS.$FABRIC_BUILD_ARCH.out
    [ -f "$EXPFILE" ] || EXPFILE=${f%.$TEST_SUFFIX}.out
    if ! cmp $TMPFILE $EXPFILE; then
      echo "FAIL $(basename $f)"
      echo "Expected output:"
      if [ -f $EXPFILE ]; then
        cat $EXPFILE
      else
        echo "(missing $EXPFILE)"
      fi
      echo "Actual output ($TMPFILE):"
      cat $TMPFILE
      echo "diff -u:"
      diff -u $EXPFILE $TMPFILE
      echo "To debug, run:"
      echo "gdb --args" $CMD
      ERROR=1
      break
    else
      echo "PASS $(basename $f)";
      rm $TMPFILE
    fi
  fi
done
