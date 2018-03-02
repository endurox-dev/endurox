#!/bin/bash

#
# (@) Create test folders (used as dev tool)
#

if [ "$#" -ne 1 ]; then
    scriptname="$(basename $0)"
    echo "Illegal number of parameters. Usage: $scriptname <dbname>"
fi

echo "Creating $1"

mkdir $1
echo > $1/.gitignore
git add $1/.gitignore

mkdir dom2/$1
echo > dom2/$1/.gitignore
git add dom2/$1/.gitignore

mkdir dom3/$1
echo > dom3/$1/.gitignore
git add dom3/$1/.gitignore


