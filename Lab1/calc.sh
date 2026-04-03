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

if [ -f "$ANS_FILE" ]; then
  ANS=$(cat "$ANS_FILE")
else
  ANS=0
  printf "%s\n" "$ANS" > "$ANS_FILE"
fi

if [ ! -f "$HIST_FILE" ]; then
  : > "$HIST_FILE"
fi

clear_screen

while true; do
  if ! IFS= read -r -p ">> " input; then
    break
  fi

  if [ "$input" = "EXIT" ]; then
    break
  fi

  if [ "$input" = "HIST" ]; then
    tail -n 5 "$HIST_FILE"
    wait_any_key
    clear_screen
    continue
  fi

  read -r -a arr <<< "$input"
  if [ "${#arr[@]}" -ne 3 ]; then
    echo "SYNTAX ERROR"
    wait_any_key
    clear_screen
    continue
  fi

  a="${arr[0]}"
  op="${arr[1]}"
  b="${arr[2]}"

  val_a="$a"
  val_b="$b"
  [ "$a" = "ANS" ] && val_a="$ANS"
  [ "$b" = "ANS" ] && val_b="$ANS"

  if ! is_number "$val_a" || ! is_number "$val_b"; then
    echo "SYNTAX ERROR"
    wait_any_key
    clear_screen
    continue
  fi

  raw_res=""

  case "$op" in
    "+")
      raw_res=$(echo "scale=8; $val_a + $val_b" | bc -l 2>/dev/null)
      ;;
    "-")
      raw_res=$(echo "scale=8; $val_a - $val_b" | bc -l 2>/dev/null)
      ;;
    "x")
      raw_res=$(echo "scale=8; $val_a * $val_b" | bc -l 2>/dev/null)
      ;;
    "/")
      if (( $(echo "$val_b == 0" | bc -l) )); then
        echo "MATH ERROR"
        wait_any_key
        clear_screen
        continue
      fi
      raw_res=$(echo "scale=8; $val_a / $val_b" | bc -l 2>/dev/null)
      ;;
    "%")
      if (( $(echo "$val_b == 0" | bc -l) )); then
        echo "MATH ERROR"
        wait_any_key
        clear_screen
        continue
      fi
      if ! is_integer "$val_a" || ! is_integer "$val_b"; then
        echo "SYNTAX ERROR"
        wait_any_key
        clear_screen
        continue
      fi
      raw_res=$(echo "$val_a % $val_b" | bc 2>/dev/null)
      ;;
    *)
      echo "SYNTAX ERROR"
      wait_any_key
      clear_screen
      continue
      ;;
  esac

  if [ -z "$raw_res" ]; then
    echo "SYNTAX ERROR"
    wait_any_key
    clear_screen
    continue
  fi

  res=$(format_result "$raw_res")
  echo "$res"

  printf "%s %s %s = %s\n" "$a" "$op" "$b" "$res" >> "$HIST_FILE"
  tail -n 5 "$HIST_FILE" > "$HIST_FILE.tmp" && mv "$HIST_FILE.tmp" "$HIST_FILE"

  ANS="$res"
  printf "%s\n" "$ANS" > "$ANS_FILE"

  wait_any_key
  clear_screen
done
