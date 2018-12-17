#!/bin/bash

if [ $# -ne 1 ]; then
echo -e "\033[0;31m[!] Usage: ./scripts/code-static-analyze.sh <SRC_ROOT> \033[0m"
exit 1
fi

#
export SRC_RT=$(cd $1 && pwd)

#
exclude_dir="3rd"
include_dir="lib/include"

#
cppcheck --enable=all -i ${exclude_dir} -I ${include_dir} ${SRC_RT}

