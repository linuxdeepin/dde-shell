#!/bin/bash
# this file is used to auto-generate .qm file from .ts file.
# author: shibowen at linuxdeepin.com

ts_list=(`ls translations/*.ts`)

for ts in "${ts_list[@]}"
do
    printf "\nprocess ${ts}\n"
    if [ -f /usr/lib/qt6/bin/lrelease ]; then
        /usr/lib/qt6/bin/lrelease "${ts}"
    elif [ -f /usr/lib64/qt6/bin/lrelease ]; then
        /usr/lib64/qt6/bin/lrelease "${ts}"
    else
        printf "Qt6 lrelease is not installed\n"
        exit 1
    fi 
done
