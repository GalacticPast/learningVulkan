#!/bin/bash

if [ -d "bin" ]; then
    rm -v bin/*
fi

if [ -d "obj" ]; then
    rm -rf -v obj/*
fi

if [ -d "src/platform/wayland" ]; then
    rm -rf -v src/platform/wayland
fi
if [ -d "assets/shaders" ]; then
    rm -rf -v assets/shaders/*.spv
fi

