#!/bin/bash
set -x

build_dir="build"
if [ ! -d "$build_dir" ]; then
    mkdir $build_dir
else
    rm -rf `pwd`/build/*
fi

cd `pwd`/build &&
    cmake .. &&
    make