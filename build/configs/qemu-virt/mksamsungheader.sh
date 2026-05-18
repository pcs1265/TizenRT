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

set -euo pipefail   # abort on error, treat unset vars as errors  


# ---------------------------------------------------------------------------  
# Directory layout (same relative structure as the original Python script)  
# ---------------------------------------------------------------------------  
# The main build process exports TOPDIR.
# However, for standalone execution, we can set a default.
THIS_PATH=`test -d ${0%/*} && cd ${0%/*}; pwd`
TOP_PATH=${THIS_PATH}/../../..

OS_DIR="${TOP_PATH}/os"  
TOOL_DIR="${OS_DIR}/tools"  
BUILD_DIR="${OS_DIR}/../build"  
OUTPUT_DIR="${BUILD_DIR}/output/bin"  

CFG_PATH="${OS_DIR}/.config"  

# ---------------------------------------------------------------------------  
# Resolve kernel binary name (fallback to tinyara.bin)  
# ---------------------------------------------------------------------------  
kernel_bin_name="tinyara.bin"  
binary_path="${OUTPUT_DIR}/${kernel_bin_name}"

# ---------------------------------------------------------------------------  
# Paths to the helper Python tools  
# ---------------------------------------------------------------------------  
mkbinheader_path="${TOOL_DIR}/mkbinheader.py"
mkchecksum_path="${TOOL_DIR}/mkchecksum.py"

# ---------------------------------------------------------------------------  
# Generate Samsung headers (same as the original os.system call)  
# ---------------------------------------------------------------------------  
python "${mkbinheader_path}" "${binary_path}" "kernel" "32"
python "${mkchecksum_path}" "${binary_path}"

echo "Header generation completed for '${binary_path}'."  