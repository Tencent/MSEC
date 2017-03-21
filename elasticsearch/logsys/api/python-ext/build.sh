#!/bin/sh

python setup.py build
cp build/*/*.so .
cp build/*/*.so ../lib
