#!/bin/bash

cd ./build
ulimit -n $(ulimit -Hn)
make test
