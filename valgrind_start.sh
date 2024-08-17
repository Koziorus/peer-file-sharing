#!/bin/bash

# Check if the script has been provided with an argument
if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <path>"
    exit 1
fi

# Store the command from the argument
PATH_TO_PROGRAM="$1"

# Run valgrind with the provided command

gnome-terminal -- bash -c "valgrind --vgdb=yes --vgdb-error=0 $PATH_TO_PROGRAM"

sleep 10