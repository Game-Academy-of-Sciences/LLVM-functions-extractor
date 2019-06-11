#/bin/bash

# This script is intended to extract LLVM bitcode and LLVM IR for anomaloues functions
# It parses anomalies.json and then uses llvm-extract to retrieve LLVM IR and LLVM bitcode
set -e;


anomalies=$(cat "$1")
llvm_bitcode_dir="${2}"
llvm_output_dir="${3}"

for anomaly in $(echo "${anomalies}" | jq -c '.[]'); do
  #echo "${anomaly}"
  _jq() {
    echo "${anomaly}" | jq -r ${1}
   }

  fun_path=$(_jq '.anomaly_name')
  fun_name=$(_jq '.full_fun_name')
  llvm_module=$(_jq '.llvm_module')
  echo "${fun_path}"
  output=$(echo "${llvm_output_dir}/${fun_path}.bc")
  #echo "${output}"
  # command=$(echo llvm-extract -func \"${fun_name}\" -o "${output}" -S "${llvm_bitcode_dir}"//"${llvm_module}")
  command='llvm-extract -func "'"${fun_name}"'"  -o '"${output}"' '"${llvm_bitcode_dir}"'/'"${llvm_module}"
  cmd=$(echo "${command}" | sed -e 's/[]$*^[]/\\&/g')
  bash -c "$cmd"

  output=$(echo "${llvm_output_dir}/${fun_path}.ll")
  command='llvm-extract -func "'"${fun_name}"'" -S -o '"${output}"' '"${llvm_bitcode_dir}"'/'"${llvm_module}"
  cmd=$(echo "${command}" | sed -e 's/[]$*^[]/\\&/g')
  bash -c "$cmd"
done
