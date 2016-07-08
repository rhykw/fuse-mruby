#!/bin/bash

set -ex

LIBMRUBY=./mruby/build/host/lib/libmruby.a
LIBS="mruby/build/host/mrbgems/mruby-redis/hiredis/lib/libhiredis.a -lm -lc"
MRUBY_INCLUDE_DIR=./mruby/include/

mkdir -p build
( rm build/* || true )

gcc -Wall -I. -I${MRUBY_INCLUDE_DIR} $(pkg-config fuse --cflags --libs) fuse-mruby.c -o build/fuse-mruby ${LIBMRUBY} $LIBS
