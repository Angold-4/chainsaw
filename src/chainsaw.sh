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

VERSION="0.5.0"
COOKIES="~/Library/Chainsaw/cookie.txt"
COUNT=0
CSRF="null";

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
    echo 'Chainsaw version 0.5.0 (Unix x86-64)'
    echo '"chainsaw help" list all avaliable commands'
fi


if [[ $# -eq 1 ]]; then
    case "$1" in
        version)
	    echo "Chainsaw 0.5.0"
	    echo "cf       ~/Library/Chainsaw/cf"
	    echo "chainsaw /usr/local/bin/chainsaw"
	    echo "csdebug  ~/Library/Chainsaw/csdebug"
	    ;;
	help)
	    echo "These are common Chainsaw commands used in various situations:"
	    echo ""
	    echo "    clean       Remove all testfile"
	    echo "    version     Check the chainsaw version"
	    echo "    help        List all valid commands"
	    echo "    login       Log into codeforces"
	    echo "    logout      Log out account"
	    echo "    check       Check your login status"
	    echo ""
	    echo "    gen         Generate problems and its testfile for specific contest"
	    echo "    runsamples  Run all tests for specific problem"
	    echo "    debug       Debug specific problem with customized input"
	    echo ""
	    echo "    submit      Submit specific problem"
	    echo ""
	    ;;
	clean)
	    rm -rf sample
	    echo "clean finished!"
	    ;;

	login)
	    # enter test logic first
	    read -p "username: " username
	    read -p "password: " password

	    cf_response=`curl --silent --cookie-jar ~/Library/Chainsaw/cookie.txt 'https://codeforces.com/enter'`
	    # output=$(g++ -std=c++17 ${2}.cpp -o sample/$2/$2 2>&1)
	    keysearch="value=\'" # TODO Maybe slow
	    rest=${cf_response#*$keysearch} # the part of cf_response after keysearch

	    CSRF=${cf_response:${#cf_response} - ${#rest}:32} # get the csrf token
	    
	    # echo -e "${GREEN}${CSRF}"

	    # request="curl --location --silent --cookie-jar ${COOKIES} --cookie ${COOKIES} "
	    # request+="--data 'action=enter&handleOrEmail=${username}&remember=1&csrf_token=${CSRF}' "
	    # request+="--data-urlencode 'password=${password}' 'https://codeforces.com/enter'"

	    # cf_response=$(curl \
	    # --location --silent --cookie-jar ${COOKIES} --cookie ${COOKIES} \
	    # --data "action=enter&handleOrEmail=${username}&remember=1&csrf_token=${CSRF}&password=${password}" \
	    # 'https://codeforces.com/enter' 2>&1)

	    # send login form 
	    cf_response=$(curl --location --silent --cookie-jar ~/Library/Chainsaw/cookie.txt --cookie ~/Library/Chainsaw/cookie.txt --data "action=enter&handleOrEmail=${username}&remember=1&csrf_token=${CSRF}&password=${password}" 'https://codeforces.com/enter/' 2>&1)

	    if [[ ${cf_response} == *"Logout"* ]];
	    then
		echo -e "${GREEN}login as ${username}${NC}"
	    else
		echo -e "chainsaw: ${RED}wrong username/password${NC}"
	    fi
	    # HttpOnly_codeforces.com	FALSE	/	FALSE	0	JSESSIONID	75EE738CE75CF23750E04FA856B25475-n1
	    # cf_response=$(curl --silent --cookie-jar ~/Library/Chainsaw/cookie.txt --cookie ~/Library/Chainsaw/cookie.txt 'https://codeforces.com/' 2>&1)

	    # echo "${cf_response}" > AngoldW.html

	    # --data-urlencode 'password=%s' '%s://%s/enter'", 
	    # g:cf_cookies_file, g:cf_cookies_file, s:cf_uname, s:cf_remember, csrf_token, s:cf_passwd, s:cf_proto, s:cf_host
	    ;;

	logout)
	    rm -f ~/Library/Chainsaw/cookie.txt
	    echo -e "${GREEN}logout successful!${NC}"
	    ;;

	check)
	    cf_response=$(curl --silent --cookie-jar ~/Library/Chainsaw/cookie.txt --cookie ~/Library/Chainsaw/cookie.txt 'https://codeforces.com/' 2>&1)
	    # userkey="<li><a href=\"/blog/"
	    # endkey=">Blog<"
	    # echo $cf_response > login.html
	    if [[ ${cf_response} == *"Logout"* ]];
	    then
		echo ${cf_response} > ~/Library/Chainsaw/temp.txt
		# find username
		# namearea=${cf_response#*$userkey}
		# endarea=${cf_response#*$endkey}
		# namel=${#namearea}
		# endl=${#endarea}
		# namelen=$(expr ${namel} - ${endl})
		# namelen=$(expr ${namelen} - 7)
		# echo -e "${GREEN}login as ${cf_response:(${#cf_response} - ${namel}):$namelen}${NC}"
		name=$(~/Library/Chainsaw/parseuser)
		echo -e "${GREEN}log in as ${name}${NC}"
		rm -f ~/Library/Chainsaw/temp.txt

	    else
		echo -e "chainsaw: ${RED}please login first${NC}"
		echo -e "run 'chainsaw login' to log in your account"
	    fi
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
	    ;;

	debug)
	    if [[ ! -e ~/debug ]]; then
	      mkdir debug
	    fi
	    ~/Library/Chainsaw/csdebug ${2}
	    ;;

	*)
	    echo "chainsaw: '$1' is not a chainsaw command. See 'chainsaw help'."
	    ;;
    esac
fi

if [[ $# -eq 3 ]]; then
    case "$1" in
	submit)
	    # submit #contest #problem

	    # very similar to login

	    # 1. check whether login
	    cf_response=$(curl --silent --cookie-jar ~/Library/Chainsaw/cookie.txt --cookie ~/Library/Chainsaw/cookie.txt 'https://codeforces.com/' 2>&1)
	    if [[ ${cf_response} != *"Logout"* ]];
	    then
		echo -e "chainsaw: ${RED}please login first${NC}"
		echo -e "run 'chainsaw login' to log in your account"
		exit 1
	    fi

	    echo ${cf_response} > ~/Library/Chainsaw/temp.txt
	    user=$(~/Library/Chainsaw/parseuser)
	    echo -e "${YELLO}${user} submiting problem $3 of contest $2 ...${NC}"
	    # echo ${user}

	    rm -f ~/Library/Chainsaw/temp.txt

	    con=$2
	    problem=$3
	    file="${problem}.cpp"
	    language='54' # cpp 17 default TODO: Add friendly user config interface

	    # 2. get the submit page (get csrf_token)
	    cf_response=$(curl --silent --cookie-jar ~/Library/Chainsaw/cookie.txt --cookie ~/Library/Chainsaw/cookie.txt "https://codeforces.com/contest/${con}/submit" 2>&1)

	    keysearch="value=\'" # TODO Maybe slow
	    rest=${cf_response#*$keysearch} # the part of cf_response after keysearch

	    CSRF=${cf_response:${#cf_response} - ${#rest}:32} # get the csrf token

	    # echo "${CSRF}"

	    # 3. submit file
	    cf_response=`curl --location --silent --cookie-jar ~/Library/Chainsaw/cookie.txt --cookie ~/Library/Chainsaw/cookie.txt -F "csrf_token=${CSRF}" -F "action=submitSolutionFormSubmitted" -F "submittedProblemIndex=${problem}" -F "programTypeId=${language}" -F "source=@${file}" "https://codeforces.com/contest/${con}/submit?csrf_token=${CSRF}"`


	    if [[ "${cf_response}" ==  *"You have submitted exactly the same code before"* ]]
	    then
		echo -e "chainsaw: ${RED}submit failed, because you have submitted exactly the same code before${NC}"
		exit 1
	    fi

	    # echo "${cf_response}" > submit.html

	    echo -e "${GREEN}submit successful, now get the verdict...${NC}"

	    sleep 2

	    # 4. check answer
	    # name=$(~/Library/Chainsaw/substring 'AngoldW.html' 2>&1)
	    # echo -e "${GREEN}${name}"

            # return verdict, contestId, index, name, passedTestCount, timeConsumedMillis, memoryConsumedBytes
	    read verdict contestId index name passedTestCount timeConsumedMillis memoryConsumedBytes <<< `~/Library/Chainsaw/parsesubmit ${user}`
	    if [[ "${verdict}" == "WRONG_ANSWER" ]] 
	    then
		COLOR=${RED};
	    else
		COLOR=${GREEN};
	    fi

	    echo ""
	    echo -e "${COLOR}${contestId}${index} ${name}"
	    echo -e "${COLOR}${verdict}"
	    echo -e "${COLOR}Number of passed test(s): ${passedTestCount}"
	    echo -e "${COLOR}Time consumed Mill(s): ${timeConsumedMillis}"
	    echo -e "${COLOR}Memory consumed byte(s): ${memoryConsumedBytes}"
	    ;;
	*)
	    echo "chainsaw: "$1" is not a chainsaw command. See 'chainsaw help'."
	    ;;
    esac
fi
