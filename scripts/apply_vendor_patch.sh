#!/usr/bin/env bash
set -eo pipefail

cd vendor/libsoundio

for f in `ls ../*.patch`
do
 echo "Patching $f"
 git apply $f || echo "Patch not applying"
 echo
done
