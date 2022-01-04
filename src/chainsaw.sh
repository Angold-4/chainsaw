#! /bin/bash

## =================================================================
## CHAINSAW COMMAND LINE TOOL
##
## This script is a chainsaw interface wrapper 
## which gives a  
##
##
## Supported environments:
##  * Ubuntu 18.04
##  * macOS
## =================================================================

VERSION="0.1.0"

if [[ $# -eq 0 ]]; then
    echo '============ Chainsaw: A Codeforces Commandline Tool =============='
    echo 'Chainsaw version 0.1.0 (x86-64)'
    echo '"chainsaw help" list all avaliable commands'
fi

# 1. Get # problems
if [[ $# -eq 1 ]]; then
    case "$1" in
        version)
	    echo "Chainsaw 0.1.0"
	    echo "cf       ~/Library/Chainsaw/cf"
	    echo "chainsaw /usr/local/bin/chainsaw"
	    ;;
	help)
	    echo "These are common Chainsaw commands used in various situations:"
	    echo ""
	    echo "    gen         Generate problems and its testfile for specific contest"
	    echo "    runsamples  run all tests for specific problem"
	    echo "    clean       Remove all testfile"
	    echo "    version     Check the chainsaw version"
	    echo "    help        List all valid commands"
	    ;;
	*)
	    echo "chainsaw: '$1' is not a chainsaw command. See 'chainsaw help'."
	    ;;
    esac
fi

if [[ $# -eq 2 ]]; then
    case "$1" in
	gen)
	    echo '============ Chainsaw: A Codeforces Commandline Tool =============='
	    echo ""
	    echo "Generating all problems and sample tests for Contest $2..."
	    echo ""

	    mkdir "sample"
	    for np in `~/Library/Chainsaw/cf 1 $2` 
	    do
		cp ~/Library/Chainsaw/Template.cpp ${np}.cpp
		mkdir "sample/${np}"
	    done
	    ~/Library/Chainsaw/cf 0 $2
	    ;;

    esac
fi




