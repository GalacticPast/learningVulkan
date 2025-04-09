#!/bin/bash

echo "building project"


platform=$(echo "$XDG_SESSION_TYPE")

#if [[ $platform == "wayland" ]]; then
#    mkdir -p src/platform/wayland
#    cd src/platform/wayland
#
#    wayland-scanner private-code  < /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml  > xdg-shell-protocol.c
#    error=$?
#    if [ $error -ne 0 ]
#    then
#        echo "Error:"$error && exit
#    fi
#
#    wayland-scanner client-header < /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml > xdg-shell-client-protocol.h 
#    error=$?
#    if [ $error -ne 0 ]
#    then
#        echo "Error:"$error && exit
#    fi
#
#    cd ../../../
#fi

make all

#./post_build.sh
