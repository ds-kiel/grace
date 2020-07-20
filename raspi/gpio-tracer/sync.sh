#! /usr/bin/env bash

rsync --filter='merge ./sync.rules' -a ./ user@192.168.1.111:/usr/testbed/gpio-tracer/
rsync --filter='merge ./sync.rules' -a ./ user@192.168.1.113:/usr/testbed/gpio-tracer/
rsync --filter='merge ./sync.rules' -a ./ user@192.168.1.104:/usr/testbed/gpio-tracer/
