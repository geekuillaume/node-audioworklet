#!/usr/bin/env bash

# This is used when working with a distant ssh peer which has rsync
# first argument needs to be the ssh connection string ([user@]host)

rsync -avz --exclude node_modules ./{src,tests,vendor,package.json,yarn.lock} $1
while inotifywait -r -e modify,create,delete,move ./src ./tests ./vendor ./package.json; do
  wait 1
  rsync -avz --exclude node_modules ./{src,tests,vendor,package.json,yarn.lock} $1
done
