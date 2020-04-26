#!/usr/bin/env bash
# set -eo pipefail

while true; do
    npm run build:debug
    inotifywait -r -e modify,create,delete,move ./src
    wait 1
done
