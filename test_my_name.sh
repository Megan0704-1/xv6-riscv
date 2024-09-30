#!/bin/bash

TOTAL_POSSIBLE_PTS=50
TOTAL_PTS=0

report_score ()
{
    echo "Final Score (my_name): ${TOTAL_PTS} / ${TOTAL_POSSIBLE_PTS}"
}

check_file ()
{
    local file_name=$(realpath "$1" 2>/dev/null)

    if [ ! -e ${file_name} ]; then
        echo "File: ${file_name} not found - aborted"
        exit 1
    fi
}

compile_module ()
{
    if ! make 1>/dev/null; then
        echo "Kernel Module does not compile"
        return 1
    fi

    return 0
}

check_module_output ()
{
    local mod_name="my_name"
    local expected="$1"

    local output=`sudo dmesg`
    echo " - expected dmesg output: ${output}"

    # Checking for successful module insertion
    if ! lsmod | grep -q "^${mod_name}"; then
        echo "Failed to insert kernel module"
        return 1
    else
        let TOTAL_PTS=TOTAL_PTS+20
    fi

    # Checking for parameters here
    if ! echo ${output} | grep -E "${expected}" 1>/dev/null; then
        echo "Output from kernel module is incorrect"
        return 1
    else
        let TOTAL_PTS=TOTAL_PTS+20
    fi

    return 0
}

unload_module ()
{
    echo "Removing kernel module"
    sudo dmesg -C && sudo rmmod ${mod_name}

    # Checking for successful module removal
    if lsmod | grep -q "^${mod_name}"; then
        echo "Failed to remove kernel module"
        return 1
    else
        let TOTAL_PTS=TOTAL_PTS+10
    fi

    return 0
}

load_module_with_params ()
{
    local mod_name="my_name"

    if [ ! -e "${mod_name}.ko" ]; then
        echo "${mod_name}.ko not found - aborted"
        exit 1
    fi

    echo 'Loading kernel module with module params: charParameter="Fall" intParameter=2024'
    sudo dmesg -C && sudo insmod "${mod_name}.ko" charParameter="Fall" intParameter=2024

    check_module_output "Hello, I am .*, a student of CSE330 Fall 2024"
    unload_module
}

check_all ()
{
    if compile_module; then
        # load_module_with_no_params
        load_module_with_params
    fi
    report_score
}

if [ "$#" -ne 1 ]; then
    echo "Usage: ./test_my_name.sh <path to directory containing my_name.c and Makefile>"
    exit 1
fi

check_file "$1/Makefile"
check_file "$1/my_name.c"

if [ -e "$1" ]; then
    pushd "$1" 1>/dev/null
    check_all
    popd 1>/dev/null
else
    echo "File $1 does not exist"
    exit 1
fi

