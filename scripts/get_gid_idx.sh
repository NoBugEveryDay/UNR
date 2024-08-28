#!/bin/bash

if [[ $UNR_DIR == "" ]]
then
    cd $(dirname "$BASH_SOURCE")/
    source ../fast/env.sh
fi

GID_IDX=`/usr/sbin/show_gids | grep v2 | grep -E '([0-9]{1,3}\.){3}[0-9]{1,3}' | cut -f 3`

# echo get_gid_idx.sh: GID_IDX = $GID_IDX

exit $GID_IDX