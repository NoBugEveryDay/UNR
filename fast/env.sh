#!/bin/bash

cd $(dirname "$BASH_SOURCE")/..
export UNR_DIR=`pwd`
cd - > /dev/null

if [[ $(whoami) == sysu_hpcscc_1 ]]
then
    export TARGET_SYSTEM=TH2K
    source /app/modules/init/bash
    export MODULEPATH=/app/modulefiles/compiler:/app/modulefiles/MPI:/app/modulefiles/application:/app/modulefiles/library:$HOME/fgn/modulefiles
    module load IMPI/2018.1.163-icc-18.0.1
    unset I_MPI_ROOT_modshare
    export UNR_INTER=verbs
    export UNR_INTRA=verbs

elif [[ $(whoami) == nsccgz_yjsb_1 ]]
then
    module load mpi/mpich/4.1.2-icx-oneapi2023.2-ch4
    export TARGET_SYSTEM=THXY
    export UNR_INTER=glex0,glex1
    export UNR_INTRA=glex0,glex1

elif [[ $HOSTNAME == cn* ]]
then
    export TARGET_SYSTEM=TH2A
    source ~/fgn/env/cn/intel-18.0.0.sh
    export UNR_INTER=glex
    export UNR_INTRA=glex
    
    `# It is used to check the UNR multi-channel`
    # export URI_GLEX_DEVICE_NUM=5
    # N=$URI_GLEX_DEVICE_NUM
    # for ((i=0; i<N; i++)); do
    #     str+="glex$i"
    #     if (( i < N-1 )); then
    #         str+=","
    #     fi
    # done
    # export UNR_INTER=$str
    # export UNR_INTRA=$str

elif [[ $HOSTNAME == FGN-* ]]
then
    export UNR_INTER=mpi
    export UNR_INTRA=mpi
elif [[ $HOSTNAME == an* ]]
then
    export TARGET_SYSTEM=THAN
    module load nvhpc/23.11
    export UNR_INTER=verbs
    export UNR_INTRA=verbs
fi

echo "export UNR_INTER=$UNR_INTER"
echo "export UNR_INTRA=$UNR_INTRA"

export PATH=$UNR_DIR/install/bin:$PATH
export CPATH=$UNR_DIR/install/include:$CPATH
export LIBRARY_PATH=$UNR_DIR/install/lib:$LIBRARY_PATH
export LD_LIBRARY_PATH=$UNR_DIR/install/lib:$LD_LIBRARY_PATH