#!/bin/bash

# Zip format compliance
TOTAL_FILES=0
TOTAL_DIRS=0
EXTRA_FILES=""

# Kernel module correctness
KERNEL_MODULE_NAME="producer_consumer"
KERNEL_MODULE_ERR=""
KERNEL_MODULE_MSG=""
KERNEL_MODULE_PTS=0
KERNEL_MODULE_TOTAL=20

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

check_file ()
{
    local file_name=$(realpath "$1" 2>/dev/null)

    if [ -e ${file_name} ]; then
        let TOTAL_FILES=TOTAL_FILES+1
        echo "[log]: - file ${file_name} found"
        return 0
    else
        return 1
    fi
}

check_dir ()
{
    local dir_name=$(realpath "$1" 2>/dev/null)

    if [ -d ${dir_name} ]; then
        let TOTAL_DIRS=TOTAL_DIRS+1
        echo "[log]: - directory ${dir_name} found"
        return 0
    else
        return 1
    fi
}

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

compile_module ()
{
    make_err=$(make 2>&1 1>/dev/null)

    if [ $? -ne 0 ] ; then
        KERNEL_MODULE_ERR="${KERNEL_MODULE_ERR}\n - (-20 points) Failed to compile your kernel module: ${make_err}"
        popd 1>/dev/null
        return 1
    fi

    echo "[log]: - Compiled successfully"
    return 0
}

load_module_with_params ()
{
    local prod=$1
    local cons=$2
    local size=$3

    # Check to make sure kernel object exists
    if [ ! -e "${KERNEL_MODULE_NAME}.ko" ]; then
        KERNEL_MODULE_ERR="${KERNEL_MODULE_ERR}\n - (-2 points) Failed to find your kernel object ${KERNEL_MODULE_NAME}.ko"
        popd 1>/dev/null
        return 1
    fi

    # Insert kernel module - check exit code
    sudo dmesg -C
    sudo insmod "${KERNEL_MODULE_NAME}.ko" prod=${prod} cons=${cons} size=${size}
    if [ $? -ne 0 ]; then
        KERNEL_MODULE_ERR="${KERNEL_MODULE_ERR}\n - (-2 points) Insmod exitted with non-zero return code"
        popd 1>/dev/null
        return 1
    fi

    # Check lsmod to make sure module is loaded
    if ! lsmod | grep -q "^${KERNEL_MODULE_NAME}"; then
        KERNEL_MODULE_ERR="${KERNEL_MODULE_ERR}\n - (-2 points) Kernel module does not appear in lsmod"
        return 1
    fi

    return 0
}

unload_module ()
{
    sudo dmesg -C && sudo rmmod "${KERNEL_MODULE_NAME}"

    # Checking for successful module removal
    if lsmod | grep -q "^${KERNEL_MODULE_NAME}"; then
        KERNEL_MODULE_ERR="${KERNEL_MODULE_ERR}\n - (-2 points) Failed to unload kernel module"
        echo "[log]: - Failed to unload kernel module"
        return 1
    fi

    return 0
}

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

check_threads ()
{
    local prod=$1
    local cons=$2

    # Check for producers
    local count=$(sudo ps aux | grep "Producer-" | wc -l)
    let count=count-1

    if [ "${count}" -ne "${prod}" ]; then
        KERNEL_MODULE_ERR="${KERNEL_MODULE_ERR}\n - (-4 points) Found ${count} producer threads, expected ${prod}"
        return 1
    fi

    # Check for consumers
    local count=$(sudo ps aux | grep "Consumer-" | wc -l)
    let count=count-1

    if [ "${count}" -ne "${cons}" ]; then
        KERNEL_MODULE_ERR="${KERNEL_MODULE_ERR}\n - (-4 points) Found ${count} consumer threads, expected ${cons}"
        return 1
    fi

    # All is good
    return 0
}

check_output ()
{
    # We use head 1000 here to prevent the rate at which `dmesg` is receiving output from your kernel
    # module from preventing the file redirection from ever seeing EOF, this way it is garunteed to
    # finish. We do not need all output to validate the correctness, the first 1000 lines is ok.
    local log_filename="output_$(date +%s).log"
    sudo dmesg | head -n 1000 > "${log_filename}"

    local prod=$1
    local cons=$2
    local size=$3

    # Check 1: If there is no space to produce items,
    # there will never be any output.
    if [ "${size}" -eq 0 ]; then
        echo "[info]: The size is zero so we will check to make sure no items are produced or consumed"

        # Check for produced items
        line_count=$(grep "An item has been produced" "${log_filename}" | wc -l)
        if [ "${line_count}" -ne 0 ]; then
            KERNEL_MODULE_ERR="${KERNEL_MODULE_ERR}\n - (-8 points) Size is zero, yet items have been produced"
            rm ${log_filename}
            return 1
        else
            rm ${log_filename}
            return 0
        fi

        # Check for consumed items
        line_count=$(grep "An item has been consumed" "${log_filename}" | wc -l)
        if [ "${line_count}" -ne 0 ]; then
            KERNEL_MODULE_ERR="${KERNEL_MODULE_ERR}\n - (-8 points) Size is zero, yet items have been consumed"
            rm ${log_filename}
            return 1
        else
            rm ${log_filename}
            return 0
        fi
    fi

    # Check 2: No producers means no output because no items will ever
    # be produced (or consumed, since there is nothing to consume)
    if [ "${prod}" -eq 0 ]; then
        echo "[info]: There are no producers, so we will make sure no items are produced or consumed"

        # Checking for any output at all - if we see any the solution is wrong
        line_count=$(grep -E "An item has been produced|An item has been consumed" "${log_filename}" | wc -l)
        if [ "${line_count}" -ne 0 ]; then
            KERNEL_MODULE_ERR="${KERNEL_MODULE_ERR}\n - (-8 points) There are no producers, yet items have been produced"
            rm ${log_filename}
            return 1
        else
            rm ${log_filename}
            return 0
        fi
    fi

    # Check 3: If there are producers, and no consumers, then the
    # total number of items produced is bound by the size
    if [ "${cons}" -eq 0 ]; then
        echo "[info]: There are no consumers, so we will make sure items are produced but not consumed"

        # Check for items produced - should equal the size
        line_count=$(grep "An item has been produced" "${log_filename}" | wc -l)
        if [ "${line_count}" -ne ${size} ]; then
            KERNEL_MODULE_ERR="${KERNEL_MODULE_ERR}\n - (-8 points) There are no consumers, yet the producers have produced more than size"
            rm ${log_filename}
            return 1
        else
            rm ${log_filename}
            return 0
        fi

        # Check for items consumed - should equal zero
        line_count=$(grep "An item has been consumed" "${log_filename}" | wc -l)
        if [ "${line_count}" -ne ${size} ]; then
            KERNEL_MODULE_ERR="${KERNEL_MODULE_ERR}\n - (-8 points) There are no consumers, yet items have been consumed"
            rm ${log_filename}
            return 1
        else
            rm ${log_filename}
            return 0
        fi
    fi

    # Check 4: If there are producers, and consumers, and there is
    # a non-zero size, there should be an ongoing cycle of production
    # and consumption. This checks to make sure that a reasonable
    # amount of production and consumption has occured.
    line_count=$(grep -E "An item has been produced|An item has been consumed" "${log_filename}" | wc -l)
    if [ "${line_count}" -le "${size}" ]; then
        KERNEL_MODULE_ERR="${KERNEL_MODULE_ERR}\n - (-8 points) The producers are not producing enough"
        rm ${log_filename}
        return 1
    fi

    rm ${log_filename}
    return 0
}

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ #

check_kernel_module ()
{
    local STATUS=0
    local prod=$1
    local cons=$2
    local size=$3

    echo "Testing your kernel module with ${prod} producers, ${cons} consumers, and a size of ${size}:"

    # Step 1: Check Makefile - stop if failed
    echo "[log]: Look for Makefile"
    if ! check_file "Makefile"; then
        KERNEL_MODULE_ERR="${KERNEL_MODULE_ERR}\n - (-20 points) Failed to find your Makefile"
        return 1
    fi

    # Step 2: Check producer_consumer.c - stop if failed
    echo "[log]: Look for source file (producer_consumer.c)"
    if ! check_file "producer_consumer.c"; then
        KERNEL_MODULE_ERR="${KERNEL_MODULE_ERR}\n - (-20 points) Failed to find your producer_consumer.c source file"
        return 1
    fi

    # Step 3: Compile the kernel module - stop if failed
    echo "[log]: Compile the kernel module"
    if ! compile_module; then
        return 1
    fi

    # Step 4: Load the kernel module - stop if failed
    echo "[log]: Load the kernel module"
    if ! load_module_with_params ${prod} ${cons} ${size}; then
        return 1
    else
        echo "[log]: - Loaded successfully"
        let KERNEL_MODULE_PTS=KERNEL_MODULE_PTS+4
    fi

    # Step 5: Check the thread count
    if ! check_threads ${prod} ${cons}; then
        let STATUS=1
    else
        echo "[log]: - Found all expected threads"
        let KERNEL_MODULE_PTS=KERNEL_MODULE_PTS+4
    fi

    # We sleep here to give the threads time to produce/consume. We need to have
    # something in dmesg to look for to make sure the behavior is as expected.
    sleep 0.1

    # Step 6: Check output
    echo "[log]: Checking output"
    if ! check_output $prod $cons $size; then
        return 1
    else
        echo "[log]: - Output is correct"
        let KERNEL_MODULE_PTS=KERNEL_MODULE_PTS+4
    fi

    # Step 7: Unload module - stop if failed
    echo "[log]: Unload the kernel module"
    if ! unload_module ${log_filename}; then
        return 1
    else
        echo "[log]: - Kernel module unloaded sucessfully"
        let KERNEL_MODULE_PTS=KERNEL_MODULE_PTS+4
    fi

    # Step 8: Make sure all threads are stopped
    echo "[log]: Checking to make sure kthreads are terminated"
    if ! check_threads 0 0; then
        let STATUS=1
    else
        echo "[log]: - All threads have been stopped"
        let KERNEL_MODULE_PTS=KERNEL_MODULE_PTS+4
    fi

    return $STATUS
}

check_zip_content ()
{
    # Step 1: Check for `source_code` directory - stop if failed
    echo "[log]: Look for directory (source_code)"
    if ! check_dir "source_code"; then
        KERNEL_MODULE_ERR="${KERNEL_MODULE_ERR} directory source_code not found"
        return 1
    fi

    # Step 2: Check Makefile - stop if failed
    echo "[log]: Look for Makefile"
    if ! check_file "source_code/Makefile"; then
        KERNEL_MODULE_ERR="${KERNEL_MODULE_ERR} file Makefile not found"
        return 1
    fi

    # Step 3: Check producer_consumer.c - stop if failed
    echo "[log]: Look for source file (producer_consumer.c)"
    if ! check_file "source_code/producer_consumer.c"; then
        KERNEL_MODULE_ERR="${KERNEL_MODULE_ERR} file "
        return 1
    fi

    return 0
}
