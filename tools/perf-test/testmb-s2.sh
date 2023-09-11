#!/bin/sh
cd  ~/DevLocal/builds/midibridge/tools/perf-test/
while true; do ./perf-test -s hw:2,0,0; sleep 0.3; done
