#!/bin/bash

if [ $# -ne 2 ]; then
echo -e "\033[0;31m[!] Usage: ./scripts/code-static-analyze.sh <SRC_ROOT> <BUILD_DIR> \033[0m"
exit 1
fi

#
export SRC_RT=$(cd $1 && pwd)
export BUILD_DIR=$(mkdir -p $2 && cd $2 && pwd)

#
source ${SRC_RT}/scripts/code-helpers.sh

# get list of checkers families
# possible values: scan-build's default c++ checkers
#  "core" "cplusplus" "deadcode" "optin.cplusplus" "security" "unix"
# source:
#  https://clang-analyzer.llvm.org/available_checks.html#default_chkr

# AND

# possible values: scan-build's alpha c++ checkers
#  "alpha.core" "alpha.cplusplus" "alpha.deadcode" "alpha.security"
#  "alpha.unix" "alpha.clone" 
# source:
#  https://clang-analyzer.llvm.org/alpha_checks.html

chkrs_fam_lst=(\
  "core" "cplusplus" "deadcode" "security" "unix" "optin.cplusplus" \
  "alpha.core" "alpha.cplusplus" "alpha.deadcode" "alpha.security" \
  "alpha.unix" "alpha.clone")


# get list of checkers
chkrs_lst=()
for chkrs_fam in ${chkrs_fam_lst[@]}; do
  echo "[DEBUG] add checker from family ${chkrs_fam} ..."
  chkrs_lst+=($(scan-build_get_checkers "${chkrs_fam}"))
done

sb_chkrs_args=""
for checker in ${chkrs_lst[@]}; do
  echo "[DEBUG] running with checker ${checker} ..."
  sb_chkrs_args+="-enable-checker ${checker} "
done

# 
IFS='' read -r -d '' sb_args <<"EOF"
  -stats --force-analyze-debug-code -o ./reports/
  -analyzer-config stable-report-filename=true"
EOF

cd ${BUILD_DIR} && rm ./* -rf
scan-build ${sb_args} \
           ${sb_chkrs_args} \
           cmake ${SRC_RT}
scan-build ${sb_args} \
           ${sb_chkrs_args} \
           make -j $(nproc --all)
