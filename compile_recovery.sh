#! /bin/bash

set -e

# Run prep_revisions.sh to generate all files
if [ ! -f "prep_done.txt" ]; then
	./prep_revisions.sh
fi

quartus_map --write_settings_files=off top -c recovery
quartus_cdb --write_settings_files=off --merge top -c recovery
quartus_fit --write_settings_files=off top -c recovery
quartus_asm --write_settings_files=off top -c recovery
quartus_sta top -c recovery
quartus_pow --write_settings_files=off top -c recovery

echo "Compilation complete"
