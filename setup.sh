source /cvmfs/sw.hsf.org/key4hep/setup.sh
export LD_LIBRARY_PATH="$(cd "$(dirname "${BASH_SOURCE[0]:-$0}")" && pwd)/plugins/build:$LD_LIBRARY_PATH"