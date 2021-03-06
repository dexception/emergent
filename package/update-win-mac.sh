#!/bin/bash
# Moves Windows and Mac emergent packages from your home directory
# into the FTP directory.  Run as root.

RUN_ON=grey
if ! grep -q "$RUN_ON" /etc/hostname; then
  echo "ERROR: This script should be run on ${RUN_ON}"
  exit
fi
if [ "`id -u`" != "0" ]; then
  echo "Running script as root..."
  sudo $0
  exit
fi

FTP_DIR=/var/ftp/pub/emergent
for f in `find . -name "emergent*-win32.exe" \
              -o -name "emergent*-win64.exe" \
              -o -name "emergent*-mac32.dmg" \
              -o -name "emergent*-mac64.dmg"`; do
  mv $f $FTP_DIR
  chmod 644 $FTP_DIR/$f
  chown root.root $FTP_DIR/$f
  ls -l $FTP_DIR/$f
done

