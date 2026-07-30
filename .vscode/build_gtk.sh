#!/usr/bin/env bash

export PATH=/ucrt64/bin:/usr/bin:$PATH

SRC="$1"
OUT="$2"

gcc "$SRC" -g -o "$OUT" \
    $(pkg-config --cflags --libs gtk+-3.0 gtksourceview-3.0)