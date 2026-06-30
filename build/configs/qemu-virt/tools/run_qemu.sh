#!/usr/bin/env bash

# ── 1️⃣ Define the prefix string shown before log messages ─────────────────────
PREFIX="[RUN_QEMU.SH]"          # Change this line if you want a different prefix.

# ── 2️⃣ Wrap the builtin echo with a helper function ───────────────
echo() {
    # $@ contains every argument passed to the function.
    # Use `command` to call the original builtin echo.
    command echo -e "\e[33m${PREFIX}\e[0m - $*"
}
# -----------------------------------------------------------

THIS_PATH=`test -d ${0%/*} && cd ${0%/*}; pwd`
TOP_PATH=${THIS_PATH}
CONFIG_FILE="${THIS_PATH}/os/.config"
PFLASH_IMAGE="qemu_flash.bin"
BLK_IMAGE="qemu_blk.bin"

# Function to get config value
get_config_value() {
    grep "^$1=" "${CONFIG_FILE}" | cut -d'=' -f2 | sed 's/"//g'
}

SMP=$(get_config_value "CONFIG_SMP")
if [ "${SMP}" == "y" ]; then
    SMP_NCPUS=$(get_config_value "CONFIG_SMP_NCPUS")
    SMP_OPTION="-smp ${SMP_NCPUS}"
    echo "SMP enabled : ${SMP_NCPUS} CPUs"
else
    echo "SMP disabled"
fi

QEMU_VIRT_VIRTIO_BLK=$(get_config_value "CONFIG_QEMU_VIRT_VIRTIO_BLK")
if [ "${QEMU_VIRT_VIRTIO_BLK}" == "y" ]; then
    if [ ! -f "$(pwd)/${BLK_IMAGE}" ]; then
        echo "ERROR: VIRTIO_BLK enabled but $(pwd)/${BLK_IMAGE} is missing"
        exit 1
    fi
    echo "VIRTIO_BLK enabled"
    QEMU_VIRT_VIRTIO_BLK_OPTION="-drive file=${BLK_IMAGE},format=raw,if=none,id=blk0 -device virtio-blk-device,drive=blk0,bus=virtio-mmio-bus.0"
fi

QEMU_VIRT_VIRTIO_NET=$(get_config_value "CONFIG_QEMU_VIRT_VIRTIO_NET")
if [ "${QEMU_VIRT_VIRTIO_NET}" == "y" ]; then
    QEMU_VIRT_VIRTIO_NET_DEVICE_NUM=$(get_config_value "CONFIG_QEMU_VIRT_VIRTIO_NET_DEVICE_NUM")
    if [ -z "${QEMU_VIRT_VIRTIO_NET_DEVICE_NUM}" ]; then
        QEMU_VIRT_VIRTIO_NET_DEVICE_NUM=0
    fi
    echo "VIRTIO_NET enabled on virtio-mmio-bus.${QEMU_VIRT_VIRTIO_NET_DEVICE_NUM}"
    QEMU_NET_OPTION="-netdev user,id=net0,hostfwd=tcp::10023-:23 -device virtio-net-device,netdev=net0,bus=virtio-mmio-bus.${QEMU_VIRT_VIRTIO_NET_DEVICE_NUM}"
else
    QEMU_NET_OPTION="-net none"
fi

QEMU_VIRT_VIRTIO_SERIAL=$(get_config_value "CONFIG_QEMU_VIRT_VIRTIO_SERIAL")
if [ "${QEMU_VIRT_VIRTIO_SERIAL}" == "y" ]; then
    QEMU_VIRT_VIRTIO_SERIAL_DEVICE_NUM=$(get_config_value "CONFIG_QEMU_VIRT_VIRTIO_SERIAL_DEVICE_NUM")
    if [ -z "${QEMU_VIRT_VIRTIO_SERIAL_DEVICE_NUM}" ]; then
        QEMU_VIRT_VIRTIO_SERIAL_DEVICE_NUM=0
    fi
    echo "VIRTIO_SERIAL enabled on virtio-mmio-bus.${QEMU_VIRT_VIRTIO_SERIAL_DEVICE_NUM}"
    echo "VIRTIO_SERIAL host endpoint: tcp:127.0.0.1:4556"
    QEMU_VIRT_VIRTIO_SERIAL_OPTION="-device virtio-serial-device,bus=virtio-mmio-bus.${QEMU_VIRT_VIRTIO_SERIAL_DEVICE_NUM} -chardev socket,id=virtser0,host=127.0.0.1,port=4556,server=on,wait=off -device virtconsole,chardev=virtser0"
fi

QEMU_VIRT_VIRTIO_SERIAL_CHAR=$(get_config_value "CONFIG_QEMU_VIRT_VIRTIO_SERIAL_CHAR")
if [ "${QEMU_VIRT_VIRTIO_SERIAL_CHAR}" == "y" ]; then
    QEMU_VIRT_VIRTIO_SERIAL_CHAR_DEVICE_NUM=$(get_config_value "CONFIG_QEMU_VIRT_VIRTIO_SERIAL_CHAR_DEVICE_NUM")
    if [ -z "${QEMU_VIRT_VIRTIO_SERIAL_CHAR_DEVICE_NUM}" ]; then
        QEMU_VIRT_VIRTIO_SERIAL_CHAR_DEVICE_NUM=0
    fi
    echo "VIRTIO_SERIAL_CHAR enabled on virtio-mmio-bus.${QEMU_VIRT_VIRTIO_SERIAL_CHAR_DEVICE_NUM}"
    echo "VIRTIO_SERIAL_CHAR host endpoint: tcp:127.0.0.1:4557"
    QEMU_VIRT_VIRTIO_SERIAL_CHAR_OPTION="-device virtio-serial-device,bus=virtio-mmio-bus.${QEMU_VIRT_VIRTIO_SERIAL_CHAR_DEVICE_NUM} -chardev socket,id=virtchar0,host=127.0.0.1,port=4557,server=on,wait=off -device virtconsole,chardev=virtchar0"
fi
# cp ./build/output/bin/tinyara.bin ./tinyara.bin

# -serial tcp::4555,server,nowait \

echo "Using pflash image: ${PFLASH_IMAGE}"
echo "Using block image: ${BLK_IMAGE}"
echo "\e[31mMachine Start!!!\e[0m"

qemu-system-arm \
-machine virt,virtualization=off,gic-version=2 \
-m 64M \
-cpu cortex-a15 \
-nographic \
${QEMU_NET_OPTION} \
-chardev stdio,id=con,mux=on \
-serial chardev:con \
-mon chardev=con,mode=readline \
-drive if=pflash,format=raw,file=${PFLASH_IMAGE} \
${QEMU_VIRT_VIRTIO_BLK_OPTION} \
${QEMU_VIRT_VIRTIO_SERIAL_OPTION} \
${QEMU_VIRT_VIRTIO_SERIAL_CHAR_OPTION} \
-s \
${SMP_OPTION} \
$@ \

# qemu-system-arm \
# -machine virt,virtualization=off,gic-version=2 \
# -m 16M \
# -cpu cortex-a7 \
# -nographic \
# -net none \
# -chardev stdio,id=mon -mon chardev=mon,mode=readline \
# -chardev socket,id=uart0,host=127.0.0.1,port=4555,server=on,wait=off \
# -serial chardev:uart0 \
# -drive if=pflash,format=raw,file=./tinyara.bin \
# -drive if=pflash,format=raw,file=./data.bin \
# -s -smp 2 $1\
