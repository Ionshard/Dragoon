#!/bin/sh

# Just a convenience wrapper for greping the project
grep -nrI --color=always $1 src/*.c src/*/*.c src/*/*.h

