#!/bin/bash

## =================================================================
## CHAINSAW INSTALLATION
##
## This script will install all the packages that are needed to
## build and run the Chainsaw Tool.
##
## Supported environments:
##  * Ubuntu 18.04
##  * macOS
## =================================================================

dir=~/Library/Chainsaw

if [[ ! -e ~/Library ]]; then
    mkdir ~/Library
fi

chmod 755 src/chainsaw.sh

if [[ -e /usr/local/bin/chainsaw ]]; then 
    sudo rm -f /usr/local/bin/chainsaw
fi
sudo cp src/chainsaw.sh /usr/local/bin/chainsaw


if [[ ! -e $dir ]]; then
    mkdir $dir
elif [[ ! -d $dir ]]; then
    echo "$dir already exists but is not a directory" 1>&2
fi

clang++ -std=c++17 src/cf.cpp -o ~/Library/Chainsaw/cf -l curl -pthread
clang++ -std=c++17 src/parsesubmit.cpp -o ~/Library/Chainsaw/parsesubmit -l curl -pthread
clang++ -std=c++17 src/parseuser.cpp -o ~/Library/Chainsaw/parseuser

sudo cp src/template.cpp ~/Library/Chainsaw/
