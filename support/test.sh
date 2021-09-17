#!/bin/sh

USAGE="Usage: test.sh [OPTION(s)]
Options:
    -g                    Dump debug info
    -h, --help            Show this help
    -l, --long            Run more test cases
    -m, --module   <name> Test only the specified module
    -r, --root-dir <path> Specify custom path to the repo root
    -t, --target   <name> Test only the specified target
    -v, --verbose         Run test with verbose output"

DEBUG=0
LONG_TEST=0
VERBOSE=0

while [ -n "$1" ]; do
  case "$1" in
    -h|--help)
      echo "$USAGE"
      exit 0
      ;;
    -g)
      DEBUG=1
      ;;
    -l|--long)
      LONG_TEST=1
      ;;
    -m|--module)
      MODULE=$2

      if [ 2 -gt $# ]; then
        echo "$0: no module was provided after '$1'"
        exit 2
      fi

      shift
      ;;
    -r|--root-dir)
      PARPROG_ROOT_DIR="$2"

      if [ 2 -gt $# ]; then
        echo "$0: no dirname was provided after '$1'"
        exit 2
      fi

      shift
      ;;
    -t|--target)
      TARGET=$2

      if [ 2 -gt $# ]; then
        echo "$0: no target was provided after '$1'"
        exit 2
      fi

      shift
      ;;
    -v|--verbose)
      VERBOSE=1
      ;;
    *)
      echo "$0: Unknown option: '$1'"
      exit 2
      ;;
  esac
  shift
done

SCRIPT_PATH=$(readlink -f "$0")

if [ -z ${PARPROG_ROOT_DIR+x} ]; then
  export PARPROG_ROOT_DIR=$(readlink -f $(dirname "$SCRIPT_PATH")/..)
fi

if [ 0 -ne $LONG_TEST ]; then
  OPTIONS="${OPTIONS} --long"
fi

if [ 0 -ne $VERBOSE ]; then
  OPTIONS="${OPTIONS} --verbose"
fi

if [ ! -z $MODULE ]; then
  OPTIONS="${OPTIONS} --module ${MODULE}"
fi

if [ ! -z $TARGET ]; then
  OPTIONS="${OPTIONS} --target ${TARGET}"
fi

if [ 0 -ne $DEBUG ]; then
  OPTIONS="${OPTIONS} --dump-config"

  echo "$0: debug info:"
  echo "SCRIPT_PATH=${SCRIPT_PATH}"
  echo "PARPROG_ROOT_DIR=${PARPROG_ROOT_DIR}"
  echo "VERBOSE=${VERBOSE}"
  echo "MODULE=${MODULE}"
  echo "OPTIONS=${OPTIONS}"
  echo ""
fi

python3 ${PARPROG_ROOT_DIR}/test/test.py ${OPTIONS} || exit 1

exit 0
