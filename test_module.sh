#!/bin/bash

# !!! DO NOT MOVE THIS FILE !!!
source utils.sh

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

run_test ()
{
    local FINAL_TOTAL=0
    local FINAL_PTS=0

    local prod=$2
    local cons=$3
    local size=$4

    local project_path=$(realpath "$1")
    pushd ${project_path} 1>/dev/null

    if check_kernel_module $prod $cons $size; then
        echo -e "[producer_consumer]: Passed with ${KERNEL_MODULE_PTS} out of ${KERNEL_MODULE_TOTAL}"
    else
        echo -e "[producer_consumer]: Failed with ${KERNEL_MODULE_PTS} out of ${KERNEL_MODULE_TOTAL} because:${KERNEL_MODULE_ERR}."
    fi

    let FINAL_TOTAL=KERNEL_MODULE_TOTAL
    let FINAL_PTS=KERNEL_MODULE_PTS
    echo "[Total Score]: ${FINAL_PTS} out of ${FINAL_TOTAL}"

    popd 1>/dev/null
}

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

if [ "$#" -ne 4 ]; then
    echo "Run the script using this command: ./test_my_name.sh /path/to/your/submission.zip prod cons size"
    exit 1
fi

if [ -e "$1" ]; then
    run_test $1 $2 $3 $4
else
    echo "File $1 does not exist"
    exit 1
fi
