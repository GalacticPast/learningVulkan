#!/bin/bash

echo building tests...

src_files=$(find tests -type f -name '*.cpp')

if [ -z "$src_files" ]; then
  echo "No .cpp files found in the source directory."
  exit 1
fi



clang++ $src_files -o bin/tests 

if [ $? -eq 0 ]; then
  echo "Compilation successful. Executable created at: $output_executable"
else
  echo "Compilation failed."
  exit 1
fi
