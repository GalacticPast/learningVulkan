#!/bin/bash

echo "building project"


CC=clang 
includes="-Isrc/ -lvulkan"
assembly="learningVulkan"
compiler_flags="-D_DEBUG -Wall -Wextra -g3 -Wconversion -Wdouble-promotion -Wno-unused-parameter -Wno-unused-function -Wno-sign-conversion -fsanitize=undefined -fsanitize-trap"
platform=$(echo "$XDG_SESSION_TYPE")

if [[ $platform == "X11" ]]; then
    #includes="$includes -lwayland-client -lrt"
    compiler_flags="$compiler_flags -DPLATFORM_LINUX_X11"
fi
if [[ $platform == "wayland" ]]; then
    mkdir -p src/platform/wayland
    cd src/platform/wayland

    wayland-scanner private-code  < /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml  > xdg-shell-protocol.c
    error=$?
    if [ $error -ne 0 ]
    then
        echo "Error:"$error && exit
    fi

    wayland-scanner client-header < /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml > xdg-shell-client-protocol.h 
    error=$?
    if [ $error -ne 0 ]
    then
        echo "Error:"$error && exit
    fi

    cd ../../../
    
    includes="$includes -lwayland-client -lrt"
    compiler_flags="$compiler_flags -DPLATFORM_LINUX_WAYLAND"
fi




c_file_names=$(find . -type f -name '*.c')

echo $CC $c_file_names -o bin/$assembly $includes $compiler_flags
time $CC $c_file_names -o bin/$assembly $includes $compiler_flags
