#! /bin/bash

set -e

# Run prep_revisions.sh to generate all files
if [ ! -f "prep_done.txt" ]; then
	./prep_revisions.sh
fi

quartus_map --write_settings_files=off top -c non_pfr
quartus_cdb --write_settings_files=off --merge top -c non_pfr
quartus_fit --write_settings_files=off top -c non_pfr
quartus_asm --write_settings_files=off top -c non_pfr
quartus_sta top -c non_pfr
quartus_pow --write_settings_files=off top -c non_pfr
quartus_cpf -c non_pfr.cof

echo "Compilation complete"
