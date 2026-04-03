#!/bin/bash
ANS_FILE=".calc_ans"
HIST_FILE=".calc_hist"

clear_screen() {
    if[ -t 1]; then
        clear
    fi
}

wait_any_key() {
    read -r -n 1 -s _
}

is_number() {
    [[ "$1" =~ ^[-+]?[0-9]+([.][0-9]+)?$ ]]
}

is_integer() {
    [[ "$1" =~ ^[-+]?[0-9]+$ ]]
}

format_result() {
    LC_ALL=C printf "%.2f" "$1" | sed 's/0*$//;s/\.$//'
}