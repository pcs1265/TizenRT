#!/usr/bin/env bash
#============================================================================  
#  Copyright 2025 Samsung Electronics All Rights Reserved.  
#  
#  Licensed under the Apache License, Version 2.0 (the "License");  
#  you may not use this file except in compliance with the License.  
#  You may obtain a copy of the License at  
#  
#      http://www.apache.org/licenses/LICENSE-2.0  
#  
#  Unless required by applicable law or agreed to in writing, software  
#  distributed under the License is distributed on an "AS IS" BASIS,  
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  
#  See the License for the specific language governing permissions and  
#  limitations under the License.  
#============================================================================  

set -e

# The main build process exports TOPDIR.
# However, for standalone execution, we can set a default.
if [ -z "$TOPDIR" ]; then
    TOPDIR=$(cd `dirname $0`/../../.. && pwd)
fi

# Source metadata files
source "${TOPDIR}/build/configs/qemu-virt/board_metadata.txt"
source "${TOPDIR}/os/.bininfo"

CONFIG_FILE="${TOPDIR}/os/.config"
PFLASH_IMAGE="${TOPDIR}/qemu_flash.bin"
BLK_IMAGE="${TOPDIR}/qemu_blk.bin"
BIN_DIR="${TOPDIR}/build/output/bin"

declare -gA PARTITION_OFFSETS
declare -gA PARTITION_COUNTS

# Function to get config value
get_config_value() {
    grep "^$1=" "${CONFIG_FILE}" | cut -d'=' -f2 | sed 's/"//g'
}

# Calculate offsets for each partition
calculate_offsets() {
    FLASH_PART_NAMES=$(get_config_value "CONFIG_FLASH_PART_NAME")
    FLASH_PART_SIZES_KB=$(get_config_value "CONFIG_FLASH_PART_SIZE")

    # Convert comma-separated strings to arrays
    local ifs_old=$IFS
    IFS=',' read -r -a names_array <<< "$FLASH_PART_NAMES"
    IFS=',' read -r -a sizes_array <<< "$FLASH_PART_SIZES_KB"
    IFS=$ifs_old

    local offset_array=()
    CURRENT_OFFSET=0
    for i in "${!names_array[@]}"; do
        NAME="${names_array[$i]}"
        SIZE_KB="${sizes_array[$i]}"
        SIZE_BYTES=$((SIZE_KB * 1024))

        if [[ ! -v PARTITION_OFFSETS[$NAME] ]]; then
            PARTITION_OFFSETS["$NAME"]=$CURRENT_OFFSET
            PARTITION_COUNTS["$NAME"]=1
        else
            PARTITION_OFFSETS["$NAME"]="${PARTITION_OFFSETS[$NAME]},$CURRENT_OFFSET"
            PARTITION_COUNTS["$NAME"]=$((PARTITION_COUNTS["$NAME"] + 1))
        fi
        
        offset_array+=(`printf "0x%X" ${CURRENT_OFFSET}`)
        CURRENT_OFFSET=$((CURRENT_OFFSET + SIZE_BYTES))
    done

    echo ""
    echo "================================ < Flash Partition Information > ================================"
    printf "\rNAME       :"
    printf '  %12s' "${names_array[@]}"
    printf "\r\nSIZE(in KB):"
    printf '  %12s' "${sizes_array[@]}"
    printf "\r\nAddr       :"
    printf '  %12s' "${offset_array[@]}"
    printf "\n"
    echo "================================================================================================="
    echo ""
}


# Create pflash and block images if they don't exist.
create_flash_image() {
    if [ ! -e "${PFLASH_IMAGE}" ]; then
        echo "Creating 64MB pflash image file filled with 0xFF: ${PFLASH_IMAGE}"
        dd if=/dev/zero bs=1M count=64 status=progress | tr '\000' '\377' > "${PFLASH_IMAGE}" \
          || { echo "Error: Failed to create ${PFLASH_IMAGE} image."; exit 1; }
    fi
    chmod 777 "${PFLASH_IMAGE}"

    if [ ! -e "${BLK_IMAGE}" ]; then
        echo "Creating 64MB virtio-blk image file filled with 0xFF: ${BLK_IMAGE}"
        dd if=/dev/zero bs=1M count=64 status=progress | tr '\000' '\377' > "${BLK_IMAGE}" \
          || { echo "Error: Failed to create ${BLK_IMAGE} image."; exit 1; }
    fi
    chmod 777 "${BLK_IMAGE}"

}

# Overwrite flash images with 0xFF
erase() {
    echo "Overwriting 64MB pflash image file with 0xFF: ${PFLASH_IMAGE}"
    dd if=/dev/zero bs=1M count=64 2>/dev/null | tr '\000' '\377' > "${PFLASH_IMAGE}" \
      || { echo "Error: Failed to overwrite ${PFLASH_IMAGE} image."; exit 1; }
    chmod 777 "${PFLASH_IMAGE}"

    echo "Overwriting 64MB virtio-blk image file with 0xFF: ${BLK_IMAGE}"
    dd if=/dev/zero bs=1M count=64 2>/dev/null | tr '\000' '\377' > "${BLK_IMAGE}" \
      || { echo "Error: Failed to overwrite ${BLK_IMAGE} image."; exit 1; }
    chmod 777 "${BLK_IMAGE}"

}

# Write BL1 to pflash. BL1 is intentionally not part of the qemu_blk.bin partition
# table.
write_bl1() {
    local bin_name="${BL1}.bin"
    local bin_path="${BIN_DIR}/${bin_name}"

    if [ -f "${bin_path}" ]; then
        echo "Writing ${bin_name} to pflash offset 0..."
        if ! dd_output=$(dd if="${bin_path}" of="${PFLASH_IMAGE}" seek=0 bs=1024 conv=notrunc 2>&1); then
            echo "Error: Failed to write ${bin_name}."
            echo "---${dd_output}"
            exit 1
        fi
        echo "${dd_output}" | tail -n 1
    else
        echo "Warning: ${bin_path} not found. Skipping ${bin_name}."
    fi
}

# Write a binary to a specific partition
write_partition() {
    local partition_name=$1
    local partition_index=$2
    local bin_path=""
    local bin_name=""

    case "$partition_name" in
        kernel)
            bin_name="${KERNEL_BIN_NAME}"
            bin_path="${BIN_DIR}/${bin_name}"
            ;; 
        common)
            bin_name="${COMMON_BIN_NAME}"
            bin_path="${BIN_DIR}/${bin_name}"
            ;; 
        app1)
            bin_name="${APP1_BIN_NAME}"
            bin_path="${BIN_DIR}/${bin_name}"
            ;; 
        bootparam)
            bin_name="${BOOTPARAM}.bin"
            bin_path="${BIN_DIR}/${bin_name}"
            ;; 
        *)
            echo "Invalid partition name: $partition_name"
            echo "Usage: $0 [all|bl1|kernel|common|app1|bootparam]"
            exit 1
            ;; 
    esac

    local offset_to_use=""
    if [ -n "${PARTITION_OFFSETS[${partition_name}]}" ]; then
        local all_offsets=${PARTITION_OFFSETS[${partition_name}]}
        local ifs_old=$IFS
        IFS=',' read -r -a offset_array <<< "$all_offsets"
        IFS=$ifs_old

        if [ ${partition_index} -lt ${#offset_array[@]} ]; then
            offset_to_use=${offset_array[${partition_index}]}
        else
            echo "Warning: Partition index ${partition_index} for '${partition_name}' is out of bounds. Max index is $((${#offset_array[@]} - 1)). Skipping."
        fi
    fi

    if [ -f "${bin_path}" ] && [ -n "${offset_to_use}" ]; then
        echo "Writing ${bin_name} to qemu_blk.bin offset ${offset_to_use} (${partition_name}[${partition_index}])... "
        local seek_blocks=$((offset_to_use / 1024))
        if ! dd_output=$(dd if="${bin_path}" of="${BLK_IMAGE}" seek="${seek_blocks}" bs=1024 conv=notrunc 2>&1); then
            echo "Error: Failed to write ${bin_name}."
            echo "---${dd_output}"
            exit 1
        fi
        echo "${dd_output}" | tail -n 1
    else
        echo "Warning: ${bin_path} not found or partition '${partition_name}[${partition_index}]' not defined. Skipping ${bin_name}."
    fi
}

# Main logic
main() {
    calculate_offsets

    local PARTITION_INDEX=0
        

    case $1 in
        all|ALL)
            # Check if any of these have multiple partitions
            if [ "${PARTITION_COUNTS[kernel]:-0}" -gt 1 ] || \
            [ "${PARTITION_COUNTS[common]:-0}" -gt 1 ] || \
            [ "${PARTITION_COUNTS[app1]:-0}" -gt 1 ]; then
                
                echo "Multiple partitions found for 'kernel', 'common', or 'app1'."
                read -p "Enter partition number to download to (e.g., 0, 1, ...): " USER_INDEX
                if [[ "$USER_INDEX" =~ ^[0-9]+$ ]]; then
                    PARTITION_INDEX=$USER_INDEX
                    echo "Will use partition number ${PARTITION_INDEX}."
                else
                    echo "Invalid input. Defaulting to partition 0."
                fi
            fi
            create_flash_image
            echo "Writing all binaries..."
            write_bl1
            write_partition "kernel" ${PARTITION_INDEX}
            write_partition "common" ${PARTITION_INDEX}
            write_partition "app1" ${PARTITION_INDEX}
            write_partition "bootparam" 0
            ;; 
        erase|ERASE)
            erase
            ;; 
        *)
            while test $# -gt 0
            do
                local current_partition=$1
                local index_to_use=0
                if [[ "$current_partition" == "bl1" ]]; then
                    create_flash_image
                    write_bl1
                    shift
                    continue
                elif [[ "$current_partition" == "kernel" || "$current_partition" == "common" || "$current_partition" == "app1" ]]; then
                    index_to_use=${PARTITION_INDEX}
                fi
                create_flash_image
                write_partition "$current_partition" "$index_to_use"
                shift
            done
            ;; 
    esac

    # if [ ! -f "${TOPDIR}/qemu_run.sh" ]; then
        cp ${TOPDIR}/build/configs/qemu-virt/tools/run_qemu.sh ${TOPDIR}/run_qemu.sh
    # fi

    echo "Successfully downloaded to QEMU pflash and virtio-blk image(s)."
    echo ""
}

main "$@"
