#!/bin/zsh


sed -i '.bak' 's/\[/\{/g;s/\]/\}/g' ./bugTracker.txt

#input=$(<./bugTracker.json); left=${input//\[/\{}; right=${left//\]/\}}; echo $right > ./bugTracker.txt
