#! /usr/bin/env bash

rsync --filter='merge ./sync.rules' -a ./ user@rasp-unlabeled-sync-node:/usr/testbed/gpio-tracer/
# rsync --filter='merge ./sync.rules' -a ./ user@192.168.1.109:/usr/testbed/gpio-tracer/
