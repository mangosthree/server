#!/bin/bash

# exclude external libraries from code style check
exclude=(
    # dirs
    "Eluna"
)

exclude_dirs=""
for dir in "${exclude[@]}"; do
    exclude_dirs+="--exclude-dir=$dir "
done

set -e

echo "Starting Codestyling Script:"
echo

declare -A singleLineRegexChecks=(
    ["[[:blank:]]$"]="Remove whitespace at the end of the lines above"
    ["\t"]="Replace tabs with 4 spaces in the lines above"
)

for check in ${!singleLineRegexChecks[@]}; do
    echo "  Checking RegEx: '${check}'"
    
    if grep -P -r -I -n $exclude_dirs ${check} src; then
        echo
        echo "${singleLineRegexChecks[$check]}"
        exit 1
    fi
done

# declare -A multiLineRegexChecks=(
#     ["\n\n\n"]="Multiple blank lines detected, keep only one. Check the files above"
# )

# for check in ${!multiLineRegexChecks[@]}; do
#     echo "  Checking RegEx: '${check}'"

#     if grep -Pzo -r -I ${check} src; then
#         echo
#         echo
#         echo "${multiLineRegexChecks[$check]}"
#         exit 1
#     fi
# done

echo
echo "Awesome! No issues..."
