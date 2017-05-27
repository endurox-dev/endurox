#!/bin/bash

echo "Starting chld3.sh"


#
# At this point we exit, but expect that chld4 is killed too
#
function clean_up {
        echo "Doing exit of chld3"
        exit
}



trap clean_up SIGINT SIGTERM

# Run off some child
chld4.sh

while [[ 1 == 1 ]]; do
        sleep 1
done

