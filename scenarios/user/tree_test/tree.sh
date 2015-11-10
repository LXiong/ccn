#!/bin/sh

pushd ../../../main
make -j17
popd
qualnet tree.config

