#!/usr/bin/env bash
# Usage : ./build.sh            (JUCE telecharge automatiquement)
#         ./build.sh /chemin/JUCE   (JUCE local)
set -euo pipefail
cd "$(dirname "$0")"

command -v faust >/dev/null || { echo "faust introuvable dans le PATH"; exit 1; }
command -v cmake >/dev/null || { echo "cmake introuvable"; exit 1; }

EXTRA=()
[ $# -ge 1 ] && EXTRA+=("-DJUCE_DIR=$1")

cmake -B build -DCMAKE_BUILD_TYPE=Release "${EXTRA[@]}"
cmake --build build --config Release -j

echo
echo "VST3 genere :"
find build -name "*.vst3" -maxdepth 4

case "$(uname)" in
  Darwin) DEST="$HOME/Library/Audio/Plug-Ins/VST3" ;;
  *)      DEST="$HOME/.vst3" ;;
esac
echo
echo "Pour l'installer :  mkdir -p \"$DEST\" && cp -R \"build/EulerDisk_artefacts/Release/VST3/Euler Disk.vst3\" \"$DEST/\""
