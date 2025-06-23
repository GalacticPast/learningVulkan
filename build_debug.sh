#!/bin/bash

echo "building project"


platform=$(echo "$XDG_SESSION_TYPE")

if [[ $platform == "wayland" ]]; then
    if [[ ! -d "app/src/platform/wayland" ]]; then
        mkdir -p app/src/platform/wayland
        cd app/src/platform/wayland

        # Function to wrap file with #ifdef/#endif
        wrap_with_guard() {
            local file="$1"
            tmp_file="${file}.tmp"
            echo "#ifdef DPLATFORM_LINUX_WAYLAND" > "$tmp_file"
            cat "$file" >> "$tmp_file"
            echo "#endif // DPLATFORM_LINUX_WAYLAND" >> "$tmp_file"
            mv "$tmp_file" "$file"
        }

        # XDG Shell Protocol Generation
        wayland-scanner private-code < /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml > xdg-shell-protocol.c
        error=$?
        if [ $error -ne 0 ]; then
            echo "Error generating xdg-shell-protocol.c: "$error && exit $error
        fi
        wrap_with_guard xdg-shell-protocol.c

        wayland-scanner client-header < /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml > xdg-shell-client-protocol.h
        error=$?
        if [ $error -ne 0 ]; then
            echo "Error generating xdg-shell-client-protocol.h: "$error && exit $error
        fi
        wrap_with_guard xdg-shell-client-protocol.h

        wayland-scanner private-code < /usr/share/wayland-protocols/unstable/xdg-decoration/xdg-decoration-unstable-v1.xml > xdg-decoration-unstable-v1-protocol.c
        error=$?
        if [ $error -ne 0 ]; then
            echo "Error generating xdg-decoration-unstable-v1-protocol.c: "$error && exit $error
        fi
        wrap_with_guard xdg-decoration-unstable-v1-protocol.c

        wayland-scanner client-header < /usr/share/wayland-protocols/unstable/xdg-decoration/xdg-decoration-unstable-v1.xml > xdg-decoration-unstable-v1.h
        error=$?
        if [ $error -ne 0 ]; then
            echo "Error generating xdg-decoration-unstable-v1.h: "$error && exit $error
        fi
        wrap_with_guard xdg-decoration-unstable-v1.h

        cd ../../../
    fi
fi


time make -j all

#./post_build.sh
