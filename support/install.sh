#!/bin/sh

targets="ALL"
build_type="Release"
src_dir="$(readlink -f $(dirname $0)/..)"
build_dir="${src_dir}/build"
install_dir="${src_dir}/install"

USAGE="Usage info: install.sh [OPTION(s)]
Options:
    -h, --help                Show this help
    -u, --use-cache           Use existing build cache. Cache from previous build wouldn't be deleted
    -t, --targets  <targets>  Targets to build
    --build-type      <type>  Build type. 'Debug' or 'Realse' value is expected
    -S, --src-dir     <path>  Path to sources directory
    -B, --build-dir   <path>  Path to build directory
    -I, --install-dir <path>  Path to install direcory
    *                         All other options are passed directly to cmake"

while [ -n "$1" ]; do
  case "$1" in
    -u|--use-cache)
      using_build_cache="1"
      ;;
    -t|--targets)
      targets="$2"
      shift
      ;;
    --build-type)
      build_type="$2"
      shift
      ;;
    -S|--src-dir)
      src_dir="$2"
      shift
      ;;
    -B|--build-dir)
      build_dir="$2"
      shift
      ;;
    -I|--install-dir)
      install_dir="$2"
      shift
      ;;
    -h|--help)
      echo "$USAGE"
      exit 0
      ;;
    *)
      additional_cmake_properties="${additional_cmake_properties} $1"
      ;;
  esac
  shift
done

if [ -z "$using_build_cache" ]; then
  rm -rf $build_dir $install_dir || exit $?
fi

mkdir -p $build_dir || exit $?

cmake_task="-D CMAKE_BUILD_TYPE=${build_type} ${additional_cmake_properties}"
cmake_task="${cmake_task} -DENABLED_PROJECTS=${targets} -B ${build_dir} -S ${src_dir}"

#TODO: find ninja more elegant
if [ -x /usr/bin/ninja ]; then
  cmake_generator="-GNinja"
fi

build_task="${build_dir}"
install_task="-DCMAKE_INSTALL_PREFIX=/ -P ${build_dir}/cmake_install.cmake"

if [ -z "${using_build_cache}" ]; then
  bash -x -c "cmake ${cmake_generator} ${cmake_task}" || exit $?
fi

bash -x -c "cmake --build ${build_task}" || exit $?
bash -x -c "DESTDIR=${install_dir} cmake ${install_task}" || exit $?

exit 0
