#!/bin/bash

if [ -d "bin" ]; then
    rm -v bin/* 
fi

if [ -d "src/platform/wayland" ]; then
    rm -rf -v src/platform/wayland 
fi

