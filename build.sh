#!/bin/sh

set -ex

CC=cc
CFLAGS='-O2 -g -Werror -Weverything -Wno-gnu-zero-variadic-macro-arguments -Wno-gnu-empty-initializer -Wno-zero-length-array'

${CC} ${CFLAGS} -o infiniteloop_test infiniteloop.c infiniteloop_test.c
./infiniteloop_test
