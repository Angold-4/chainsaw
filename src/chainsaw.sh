#! /bin/bash

## =================================================================
## CHAINSAW COMMAND LINE TOOL
##
## This script is a chainsaw interface wrapper 
## which gives a user friendly using experience
##
## Supported environments:
##  * Ubuntu 18.04
##  * macOS
## =================================================================

VERSION="0.1.0"
COUNT=0
SUCCESS=0;
CONS=1

# Black        0;30     Dark Gray     1;30
# Red          0;31     Light Red     1;31
# Green        0;32     Light Green   1;32
# Brown/Orange 0;33     Yellow        1;33
# Blue         0;34     Light Blue    1;34
# Purple       0;35     Light Purple  1;35
# Cyan         0;36     Light Cyan    1;36
# Light Gray   0;37     White         1;37

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLO='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color


if [[ $# -eq 0 ]]; then
    echo -e "============ ${YELLO}Cha${BLUE}in${RED}saw${NC}: A Codeforces Commandline Tool =============="
    echo "
    _________ .__           .__                              
    \_   ___ \|  |__ _____  |__| ____   ___________ __  _  __
    /    \  \/|  |  \\__  \ |  |/    \ /  ___/\__  \\ \/ \/ /
    \     \___|   Y  \/ __ \|  |   |  \\___ \  / __ \\     / 
     \______  /___|  (____  /__|___|  /____  >(____  /\/\_/  
            \/     \/     \/        \/     \/      \/        
	"
    
    echo ""
    echo 'Chainsaw version 0.1.0 (Unix x86-64)'
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
	    echo "    submit      Submit specific question"
	    echo "    clean       Remove all testfile"
	    echo "    version     Check the chainsaw version"
	    echo "    help        List all valid commands"
	    echo "    login       Login to codeforces"
	    ;;
	clean)
	    rm -rf sample
	    echo "clean finished!"
	    ;;
	*)
	    echo "chainsaw: '$1' is not a chainsaw command. See 'chainsaw help'."
	    ;;
    esac
fi

if [[ $# -eq 2 ]]; then
    case "$1" in
	gen)
	    echo -e "============ ${YELLO}Cha${RED}in${BLUE}saw${NC}: A Codeforces Commandline Tool =============="
	    echo ""
	    echo "Generating all problems and sample tests for Contest $2..."
	    echo ""

	    mkdir "sample"
	    # 1. Create file and dir
	    for np in `~/Library/Chainsaw/cf $2` 
	    do
		cp ~/Library/Chainsaw/template.cpp ${np}.cpp
		mkdir "sample/${np}"
		PROBNAMES[COUNT]=$np
		COUNT=`expr $COUNT + $CONS`
	    done
	    echo "Find $COUNT problems, now generating sample tests..."

	    # 2. Write test file
	    `~/Library/Chainsaw/cf $2 "${PROBNAMES[@]}"`

	    echo ""
	    echo "Generating successfully!"
	    echo ""
	    echo "Run 'chainsaw runsamples problem' to run the tests"
	    echo ""
	    echo '==================================================================='
	    ;;

	runsamples)
	    echo ""
	    echo "Compiling ${2}.cpp..."
	    echo ""
	    # Compile Program
	    output=$(g++ -std=c++17 ${2}.cpp -o sample/$2/$2 2>&1)
	    if [[ $? != 0 ]]; then
		# There was an error, display the error in $output
		echo -e "${RED}chainsaw: Compile Error${NC}"
		exit 1
	    fi

	    # Count number of tests
	    ret=$(ls sample/$2/ | wc -l)
	    ret=${ret: -1}
	    ret=`expr $ret - 1`
	    number=`expr $ret / 2`
	    green=`tput setaf 2`

	    if [[ $number -eq 1 ]]
	    then
		echo "Run $number sample test of problem $2..."
	    else 
		echo "Run $number sample tests of problem $2..."
	    fi

	    for (( i=1; i <= $number; ++i )) {
		out=$(sample/${2}/${2} < sample/${2}/input${i}.txt 2>&1)
		in=$(cat sample/${2}/input${i}.txt 2>&1)
		ans=$(cat sample/${2}/output${i}.txt 2>&1)

		echo -e "${YELLO}TestCase: ${i}/${number} ---------------------------------"

		echo -e "${NC}Input:"
		echo -e "${NC}${in}"
		echo ""
		if [[ $out == $ans ]]
		then
		    SUCCESS=`expr $SUCCESS + $CONS`
		    echo -e "${GREEN}Output:"
		    echo -e "${GREEN}$out"
		else
		    echo -e "${RED}Output:"
		    echo -e "${RED}$out"
		fi
		echo ""
		echo -e "${NC}Expected:"
		echo -e "${NC}$ans"

		echo ""
		if [[ $out == $ans ]]
		then
		    echo -e "${GREEN}Correct"
		else 
		    echo -e "${RED}Incorrect"
		fi
	    }
	    
	    echo ""
	    if [[ $SUCCESS == $number ]]
	    then
		echo -e "${GREEN}Summary: ---------------------------------------------"
		echo -e "${GREEN} All ${SUCCESS}/${number} Passed!"
	    else
		echo -e "${RED}Summary: ---------------------------------------------"
		echo -e "${RED} ${SUCCESS}/${number} test(s) passed."
	    fi
    esac
fi



