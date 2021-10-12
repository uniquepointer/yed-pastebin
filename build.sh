#!/usr/bin/env bash
gcc -o pastebin.so pastebin.c $(yed --print-cflags) $(yed --print-ldflags)
