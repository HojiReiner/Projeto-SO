#!/bin/bash
#Script to run tests

if [ "$#" != "3" ]; then
    echo "Wrong number of arguments"
    exit 1
fi

make &>/dev/null

if [ ! -d "$2" ];then
    mkdir $2
fi

for InputFile in $(ls $1/*.txt); do
    
    for Threads in $(seq 1 ${3});do
        echo -e "\e[1;33mInputFile=$InputFile NumberThreads=$Threads \e[0m"
        
        file=$(basename $InputFile .txt)
        ./tecnicofs ${InputFile} ${2}/${filr}-${Threads}.txt ${Threads} mutex | grep "TecnicoFS"
    done

done