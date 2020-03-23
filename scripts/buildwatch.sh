#!/usr/bin/env bash
set -eo pipefail

npm run rebuild

while inotifywait -r -e modify,create,delete,move ./src; do
    wait 1
    npm run rebuild
done
