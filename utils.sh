#!/bin/bash

# This is the username of the user we will create and use to test
USERNAME="TestP4"

# Kernel module correctness
KERNEL_MODULE_NAME="producer_consumer"
KERNEL_MODULE_ERR=""
KERNEL_MODULE_PTS=0
KERNEL_MODULE_TOTAL=2500

# Function to check if comm output is empty and print "None" if so
check_output() {
    output="$1"
    if [ -z "$output" ]; then
        echo "None"
    else
        echo "$output"
    fi
}

function arraydiff() {
   awk 'BEGIN{RS=ORS=" "}
        {NR==FNR?a[$0]++:a[$0]--}
        END{for(k in a)if(a[k])print k}' <(echo -n "${!1}") <(echo -n "${!2}")
}

check_file ()
{
    local file_name=$(realpath "$1" 2>/dev/null)

    if [ -e ${file_name} ]; then
        echo "[log]: ─ file ${file_name} found"
        return 0
    else
        return 1
    fi
}

check_dir ()
{
    local dir_name=$(realpath "$1" 2>/dev/null)

    if [ -d ${dir_name} ]; then
        echo "[log]: ─ directory ${dir_name} found"
        return 0
    else
        return 1
    fi
}

compile_module ()
{
    make_err=$(make 2>&1 1>/dev/null)

    if [ $? -ne 0 ] ; then
        KERNEL_MODULE_ERR="${KERNEL_MODULE_ERR}\n - Failed to compile your kernel module: ${make_err}"
        return 1
    fi

    echo "[log]: ─ Compiled successfully"
    return 0
}

load_module_with_params ()
{
    local prod=$1
    local cons=$2
    local size=$3
    local uid=$4

    # Check to make sure kernel object exists
    if [ ! -e "${KERNEL_MODULE_NAME}.ko" ]; then
        KERNEL_MODULE_ERR="${KERNEL_MODULE_ERR}\n - Failed to find your kernel object ${KERNEL_MODULE_NAME}.ko"
        popd 1>/dev/null
        return 1
    fi

    # Insert kernel module - check exit code
    sudo dmesg -C
    sudo insmod "${KERNEL_MODULE_NAME}.ko" prod=${prod} cons=${cons} size=${size} uid=${uid}
    if [ $? -ne 0 ]; then
        KERNEL_MODULE_ERR="${KERNEL_MODULE_ERR}\n - Insmod exitted with non-zero return code"
        popd 1>/dev/null
        return 1
    fi

    # Check lsmod to make sure module is loaded
    if ! lsmod | grep -q "^${KERNEL_MODULE_NAME}"; then
        KERNEL_MODULE_ERR="${KERNEL_MODULE_ERR}\n - Kernel module does not appear in lsmod"
        return 1
    fi

    return 0
}

check_threads ()
{
    local prod=$1
    local cons=$2
    local points=$3

    # Check for producers
    local count=$(sudo ps aux | grep "Producer-" | wc -l)
    let count=count-1

    if [ "${count}" -ne "${prod}" ]; then
        KERNEL_MODULE_ERR="${KERNEL_MODULE_ERR}\n - Found ${count} producer threads, expected ${prod} (-${points} points)"
        return 1
    fi

    # Check for consumers
    local count=$(sudo ps aux | grep "Consumer-" | wc -l)
    let count=count-1

    if [ "${count}" -ne "${cons}" ]; then
        KERNEL_MODULE_ERR="${KERNEL_MODULE_ERR}\n - Found ${count} consumer threads, expected ${cons} (-${points} points)"
        return 1
    fi

    # All is good
    return 0
}

start_processes ()
{
    local regular=$1
    local zombies=$2

    for ((i=0; i<regular; i++)); do
        sudo -u "$USERNAME" bash -c "procgen regular" 2>/dev/null &
    done

    for ((i=0; i<zombies; i++)); do
        #sudo -u "$USERNAME" bash -c "procgen zombie" 2>/dev/null &
	sudo -u "$USERNAME" bash -c "procgen zombie" 2>&1 | sudo tee -a /home/$USERNAME/zombies.txt > /dev/null &
    done
}

compare_pids ()
{
    local status=0
    local final=0

    local prod=$1
    local cons=$2
    local regular=$3
    local zombies=$4

    # Base is short for "baseline"
    local base_regular=$5
    #local base_ppid_zombies=$6
    local zombie_pids=$6

    local num_zombies_killed=0
    local num_innocent_alive=0

    pattern="has consumed a zombie process with pid [0-9]+ and parent pid [0-9]+" 
    local pids_killed=($(sudo dmesg | grep -E "$pattern" | awk '{ print $(NF-4)}' | sort))
    local regular_pids_alive=($(ps -u "$USERNAME" -f | grep -E "procgen regular" | awk '{ print $2 }' | sort | uniq))
    local zombie_pids_alive=($(ps -u "$USERNAME" -f | grep -E "procgen" | grep -E "defunct" | awk '{ print $2 }' | sort | uniq))

    # Check killed zombies
    local zombie_pids_alive_len=$(echo "${zombie_pids_alive[*]}" | wc -w)
    if [[ "$zombie_pids_alive_len" -ne 0 ]]; then
        echo "[log]: ┬ Your kernel module did not properly handle all zombie processes:"
	    echo "[log]: ├─ ($zombie_pids_alive_len/$zombies) zombie processes not cleaned."
	    echo "[log]: └─ Kindly look into these pids: ${zombie_pids_alive[*]}"

        local zombie_pids_len=$(echo "${zombie_pids_alive[*]}" | wc -w)
        diff=($(arraydiff zombie_pid[@] zombie_pids_alive[@]))
        local diff_len=$(echo "${diff[*]}" | wc -w)
        local score=$(((zombie_pids_len - diff_len) * 2000 / zombie_pids_len))
        let final=final+score

        local deduct_big=$((2000-score))
        local deduct=$(echo "scale=2; ${deduct_big} / 100" | bc)
        KERNEL_MODULE_ERR="${KERNEL_MODULE_ERR}\n - Zombie processese handled improperly (-${deduct} points)"
        let status=1
    else
        echo "[log]: ─ All zombies were cleaned"
        let final=final+2000
    fi

    # Check alive regular processes
    local base_regular_len=$(echo "${base_regular[*]}" | wc -w)
    if [ "${base_regular[*]}" != "${regular_pids_alive[*]}" ]; then
        diff=($(arraydiff base_regular[@] regular_pids_alive[@]))
        echo "[log]: ┬ Your kernel module did not properly handle all regular processes:"
        echo "[log]: ├─ ($(echo '$diff' | wc -w)/$base_regular_len) regular processes incorrectly killed."
        echo "       └─ Kindly look into the following pids: ${diff[*]}"

        local base_regular_len=$(echo "${base_regular[*]}" | wc -w)
        local diff_len=$(echo "${diff[*]}" | wc -w)
        local score=$(((base_regular_len - diff_len) * 2000 / base_regular_len))

        local deduct_big=$((2000-score))
        local deduct=$(echo "scale=2; ${deduct_big} / 100" | bc)
        let final=final-deduct_big

        KERNEL_MODULE_ERR="${KERNEL_MODULE_ERR}\n - Regular processese handled improperly (-${deduct} points)"
        let status=1
    else
        echo "[log]: ─ None of the regular processes were killed"
    fi 

    if [ "${final}" -lt 0 ]; then
        let final=0
    fi
    let KERNEL_MODULE_PTS=KERNEL_MODULE_PTS+final

    return ${status}
}

unload_module ()
{
    sudo dmesg -C && sudo rmmod "${KERNEL_MODULE_NAME}"

    # Checking for successful module removal
    if lsmod | grep -q "^${KERNEL_MODULE_NAME}"; then
        KERNEL_MODULE_ERR="${KERNEL_MODULE_ERR}\n ─ Failed to unload kernel module"
        echo "[log]: ─ Failed to unload kernel module"
        return 1
    fi

    return 0
}
