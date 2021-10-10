#!/bin/zsh

input=$(<./bugTracker.json); left=${input//\[/\{}; right=${left//\]/\}}; echo $right > ./bugTracker.txt
