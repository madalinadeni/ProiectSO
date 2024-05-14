#!/bin/bash

for file in "$@"; do
    min_lines=3
    min_words=1000
    min_chars=2000
   
   keywords=('corrupted' 'dangerous' 'risk' 'attack' 'malware' 'malicious')
   
   
    if [ ! -f "$file" ]; then
        echo "Fișierul $file nu există!"
        continue
    fi

    
    num_lines=$(wc -l < "$file")
    num_words=$(wc -w < "$file")
    num_chars=$(wc -m < "$file")

    if [ $num_lines -lt $min_lines ] || [ $num_words -gt $min_words ] || [ $num_chars -gt $min_chars ]; then
        echo "Fișierul $file este considerat periculos!"
    else
        if [[ $(file "$file") != *"ASCII"* ]]; then
            echo "Fișierul $file conține caractere non-ASCII și este considerat periculos!"
            continue
        fi

        for keyword in "${keywords[@]}"; do
            if grep -q "\<$keyword\>" "$file"; then
                echo "Fișierul $file conține cuvântul cheie '$keyword' și este considerat periculos!"
                break
            fi
        done
    fi
done

