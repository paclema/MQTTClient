#!/bin/bash

# Define a function to compare version numbers
function compare_versions {
  # Extract the version numbers from the input strings fi they com wi
  v1=$(echo $1 | sed 's/v//')
  v2=$(echo $2 | sed 's/v//')

  # Split the version numbers into arrays
  IFS='.' read -r -a v1_arr <<< "$v1"
  IFS='.' read -r -a v2_arr <<< "$v2"

  # Check when this is the first version and last version is empty or 0.0.0
  if [[ -z "$v2" || "$v2" == "0.0.0" ]]; then
    latest=${v2:-empty}
    echo "New version: v$1   -   Latest version: $latest"
    exit 0
  fi

  # Compare the major, minor, and patch components in order
  if [ ${v1_arr[0]} -gt ${v2_arr[0]} ]; then
    echo "New version: v$1   -   Latest version: v$2"
    exit 0
  elif [ ${v1_arr[0]} -lt ${v2_arr[0]} ]; then
    echo "Error: The new version (v$1) is not greater than latest version (v$2)"
    exit 1
  elif [ ${v1_arr[1]:-0} -gt ${v2_arr[1]:-0} ]; then
    echo "New version: v$1   -   Latest version: v$2"
  elif [ ${v1_arr[1]:-0} -lt ${v2_arr[1]:-0} ]; then
    echo "Error: The new version (v$1) is not greater than latest version (v$2)"
    exit 1
  elif [ ${v1_arr[2]:-0} -gt ${v2_arr[2]:-0} ]; then
    echo "New version: v$1   -   Latest version: v$2"
  elif [ ${v1_arr[2]:-0} -lt ${v2_arr[2]:-0} ]; then
    echo "Error: The new version (v$1) is not greater than latest version (v$2)"
    exit 1
  else
    echo "$1 is equal to $2"
    exit 1
  fi
}
# Example usage
#                     new     last
# compare_versions "v2.0.0" "v1.7.7"
# compare_versions "2.0.0" "1.7.7"
compare_versions $1 $2