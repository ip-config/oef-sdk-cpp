#!/bin/bash

if [ $# -ne 1 ]; then
echo -e "\033[0;31m[!] Usage: ./scripts/code-format-apply.sh SRC_ROOT  \033[0m"
exit 1
fi

export SRC_RT=$1
export SRC_dirs="${SRC_RT}/lib ${SRC_RT}/apps"
SRC_files=()
SRC_files+=$(find ${SRC_dirs} -name "*.cpp" -print -o \
                              -name "*.h" -print -o \
                              -name "*.hpp" -print)


for file in ${SRC_files[@]}; do
  echo "[DEBUG] processing file ${file} ..."
  #clang-format --style=file  ${file}
done
