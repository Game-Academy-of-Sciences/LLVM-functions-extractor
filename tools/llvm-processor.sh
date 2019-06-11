#/bin/bash
set -e;

llvm_ir_dir="$1" 
llvm_bitcode_dir="$2"
echo "$llvm_ir_dir"

# Loop over LLVM IR to compile them in LLVM bitcode (using llvm-as) and store in llvm_bitcode_dir
for i in $(ls "$llvm_ir_dir");do
   if [[ $i =~ \.ll$ ]]; then
        filename=${i::-3}
        echo "$filename"
        llvm-as -o "$llvm_bitcode_dir/$filename.bc" "$llvm_ir_dir/$filename.ll" || EXIT_CODE=$? && true ;
    else
        echo "$i"
    fi

done

# Loop over LLVM bitcode files to extract functions instructions
for file in $(ls "$llvm_bitcode_dir"); do
    if [[ $file =~ \.bc$ ]]; then
        echo "$file"
        /root/llvm/openCLFeatureExtract/oclFeatureExt.out -f "$llvm_bitcode_dir/$file" -o /root/Dataset_functions || EXIT_CODE=$? && true ;
        echo "$llvm_bitcode_dir/$file"
    fi
done
