#!/bin/bash

set -e

if [[ $UNR_DIR == "" ]]
then
    cd $(dirname "$BASH_SOURCE")/
    source ./env.sh
fi

if [[ $UNR_DIR == "" ]]
then
    exit 1
fi

rm -rf $UNR_DIR/build $UNR_DIR/install
mkdir $UNR_DIR/build
cd $UNR_DIR/build

if [[ $(whoami) == sysu_hpcscc_1 ]]
then
    CMAKE_C_COMPILER=mpiicc
    CMAKE_CXX_COMPILER=mpiicpc
    CMAKE_C_FLAGS="-O3 -xhost -std=gnu99"
    MPI_C_COMPILER=mpiicc
    MPI_CXX_COMPILER=mpiicpc
elif [[ $HOSTNAME == FGN-* ]]
then
    CMAKE_C_COMPILER=mpicc
    CMAKE_CXX_COMPILER=mpicxx
    CMAKE_C_FLAGS="-O3 -g"
    MPI_C_COMPILER=mpicc
    MPI_CXX_COMPILER=mpicxx
elif [[ $HOSTNAME == cn* ]]
then
    CMAKE_C_COMPILER=mpicc
    CMAKE_CXX_COMPILER=mpicxx
    CMAKE_C_FLAGS="-O3 -xhost -std=gnu99"
    MPI_C_COMPILER=mpicc
    MPI_CXX_COMPILER=mpicxx
    CONFIG+=" -DUSE_GLEX=ON -DGLEX_VERSION=2 "
elif [[ $(whoami) == nsccgz_yjsb_1 ]]
then
    CMAKE_C_COMPILER=mpicc
    CMAKE_CXX_COMPILER=mpicxx
    CMAKE_C_FLAGS="-O3 -xhost -std=gnu99"
    MPI_C_COMPILER=mpicc
    MPI_CXX_COMPILER=mpicxx
    CONFIG+=" -DUSE_GLEX=ON -DUSE_VERBS=OFF -DGLEX_VERSION=3 "
elif [[ $HOSTNAME == an* ]]
then
    export OMPI_CC=gcc
    CMAKE_C_COMPILER=mpicc
    CMAKE_CXX_COMPILER=mpicxx
    CMAKE_C_FLAGS="-O3 -std=gnu99"
    MPI_C_COMPILER=mpicc
    MPI_CXX_COMPILER=mpicxx
    CONFIG+=" -DUSE_VERBS=ON "
else
    echo "Unknown target"
    exit 1
fi

CONFIG+=" -DASSERT_CHECK=ON -DPRINT_LOG=OFF "

cmake \
    -DCMAKE_INSTALL_PREFIX=$UNR_DIR/install \
    -DCMAKE_C_COMPILER=$CMAKE_C_COMPILER \
    -DCMAKE_CXX_COMPILER=$CMAKE_CXX_COMPILER \
    -DCMAKE_C_FLAGS="$CMAKE_C_FLAGS" \
    -DMPI_C_COMPILER=$MPI_C_COMPILER \
    -DMPI_CXX_COMPILER=$MPI_CXX_COMPILER \
    $CONFIG \
    ..  

make -j install
make test
