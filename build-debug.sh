#!/bin/bash

echo "building project"


CC=clang 
includes="-Isrc/ -lvulkan"
assembly="learningVulkan"
compiler_flags=" -Wall -Wextra -g3 -Wconversion -Wdouble-promotion -Wno-unused-parameter -Wno-unused-function -Wno-sign-conversion -fsanitize=undefined -fsanitize-trap"
defines="-D_DEBUG"

platform=$(echo "$XDG_SESSION_TYPE")

echo "Building for linux-$platform..."

if [[ $platform == "x11" ]]; then
    includes="$includes -lX11 -lxcb -lX11-xcb -L/usr/X11R6/lib"
    defines="$defines -DPLATFORM_LINUX_X11"
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
    defines="$defines -DPLATFORM_LINUX_WAYLAND"
fi




c_file_names=$(find . -type f -name '*.c')

echo $CC $c_file_names $defines -o bin/$assembly $includes $compiler_flags
time $CC $c_file_names $defines -o bin/$assembly $includes $compiler_flags
