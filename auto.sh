#!/bin/bash

printf "##################\n"
printf "# COMPIL ARDUINO #\n"
printf "##################\n"

function cleanInsideFolder() {
  if [ "$1" != "" ]; then
    rm -rf "$1"/*
  else
    echo "fichier vide erreur"
  fi
}

for i in "$@"; do
  case $i in
    f=*)
      FOLDER="${i#*=}"
      shift
    ;;
    --init)
      INIT=1
      shift
    ;;
  esac
done

if [ -z "$FOLDER" ]; then
  echo 'Aucun dossier spécifié, indiquer f=<nom_du_dossier>'
  exit 1
fi

if [ ! -d "$FOLDER" ]; then
  echo "Le dossier $FOLDER n'existe pas"
  exit 1
fi

printf 'P1 \n'
cd "$FOLDER" || exit
if [ ! -d ".tmp" ]; then
  mkdir .tmp
else
  cleanInsideFolder ".tmp"
fi
printf '.'
if [ ! -d "build" ]; then
  mkdir build
else
  cleanInsideFolder "build"
fi
printf '.'
printf 'done [env ready]\n'

compiled_ino=0

for ino in *.ino; do
  [ -f "$ino" ] || continue
  inoFile=$(basename "$ino" .ino)
  compile_output=$(/home/binz/bin/arduino-cli compile --fqbn arduino:avr:uno --output-dir build "$ino" 2>&1)
  echo "$compile_output"
  if [ -f "build/$inoFile.ino.elf" ] && [ -f "build/$inoFile.ino.hex" ]; then
    compiled_ino=1
    OUTPUT="build/$inoFile.ino" 
    break
  fi
done

if [ $compiled_ino -eq 0 ]; then
  echo "pas de .ino compilé."
  exit 1
fi

printf "Voulez-vous commencer le téléversement ? [O/N]: "
read -r TELEV
if [[ $TELEV == "N" ]]; then
  printf "Bye bye\n"
  exit 0
else
  printf "Démarrage du téléversement\n"
  printf "Liste des périphériques USB:\n"
  lsusb
  printf "Quel port utilisez-vous ? [/dev/ttyACM0]: "
  read -r PORT
  PORT=${PORT:-"/dev/ttyACM0"}
  avrdude -F -V -c arduino -p atmega328p -P "$PORT" -b 9600 -U flash:w:"${OUTPUT}.hex" || { echo "Erreur d'ouverture de $PORT"; exit 1; }
fi
