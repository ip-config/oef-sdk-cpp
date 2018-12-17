#!/bin/bash

if [ $# -ne 2 ]; then
echo -e "\033[0;31m[!] Usage: ./scripts/code-static-analyze.sh <SRC_ROOT> <BUILD_DIR> \033[0m"
exit 1
fi

# 
source $1/scripts/code-helpers.sh

export SRC_RT=$1
export SRC_dirs="${SRC_RT}lib ${SRC_RT}apps"
export header_dir_reg="${SRC_RT}lib/include/"
export header_exclude="[{\"name\":\"${SRC_RT}/3rd/*\"}]"

SRC_files=()
SRC_files+=$(list_cpp_files "${SRC_dirs}")


for file in ${SRC_files[@]}; do
  echo "[DEBUG] processing file ${file} $2 ${header_dir_reg} ..."
  clang-tidy -config=""  $file -checks=* -p $2 -header-filter="${header_dir_reg}" -line-filter=""
  #clang-tidy -config=""  $file -checks="" -p $2 -header-filter="${header_dir_reg}" -line-filter=""
done

