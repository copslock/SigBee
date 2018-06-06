#!/bin/sh
BUILD_DATE=`/bin/date +%Y%m%d`
BUILD_TIME=`/bin/date +%k%M%S`
make tes "BUILD_DATE=$BUILD_DATE" "BUILD_TIME=$BUILD_TIME"
