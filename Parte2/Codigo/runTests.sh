#!/bin/bash
#Script to run tests

if [[ "$#" != "3" ]]; then
    echo "Wrong number of arguments"
    exit 1

elif [[ ! -d "$1" ]];then
    echo "Input directory doesn't exist"
    exit 1

elif [[ ! -d "$2" ]];then
    echo "Output directory doesn't exist"
    exit 1

elif ! [[ "$3" =~ ^[0-9]+$ ]]; then
    echo "Impossible number of threads"
    exit 1

elif [[ "$3" -lt "1" ]];then
    echo "Impossible number of threads"
    exit 1

fi

for InputFile in $(ls $1/*.txt); do
    
    for Threads in $(seq 1 $3);do
        file=$(basename $InputFile .txt)
        echo -e "\e[1;33mInputFile=$file NumberThreads=$Threads \e[0m"
        
        ./tecnicofs ${InputFile} ${2}/${file}-${Threads}.txt ${Threads} | grep "TecnicoFS"
    done

done