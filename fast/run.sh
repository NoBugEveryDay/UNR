#!/bin/bash

set -e

if [[ $UNR_DIR == "" ]]
then
    cd $(dirname "$BASH_SOURCE")/
    source ./env.sh
fi


cd $UNR_DIR/build

make -j install
make test