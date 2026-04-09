#!/bin/bash
ANS_FILE=".calc_ans"
HIST_FILE=".calc_hist"

#Xóa màn hình nếu đang chạy trong terminal
clear_screen() {
    if [ -t 1 ]; then
        clear
    fi
}

#Đợi người dùng nhấn một phím bất kỳ
wait_any_key() {
    read -r -n 1 -s _
}

#Kiểm tra số có phải là số hợp lệ (có thể là số nguyên hoặc số thực)
is_number() {
    [[ "$1" =~ ^[-+]?[0-9]+([.][0-9]+)?$ ]]
}

#Kiểm tra số có phải là số nguyên hay không (dùng cho phép toán %)
is_integer() {
    [[ "$1" =~ ^[-+]?[0-9]+$ ]]
}

#Định dạng kết quả để loại bỏ các số 0 thừa sau dấu thập phân và dấu chấm nếu không còn số nào sau đó
format_result() {
    LC_ALL=C printf "%.2f" "$1" | sed 's/0*$//;s/\.$//'
}

#Khởi tạo biến ANS từ file nếu tồn tại, nếu không thì đặt mặc định là 0
if [ -f "$ANS_FILE" ]; then
  ANS=$(cat "$ANS_FILE")
else
  ANS=0
  printf "%s\n" "$ANS" > "$ANS_FILE"
fi

#Khởi tạo file lịch sử nếu chưa tồn tại
if [ ! -f "$HIST_FILE" ]; then
  : > "$HIST_FILE"
fi

#Xóa màn hình trước khi bắt đầu vòng lặp chính
clear_screen

#Vòng lặp chính để đọc và xử lý lệnh từ người dùng
while true; do
  #Đọc lệnh từ người dùng
  if ! IFS= read -r -p ">> " input; then
    break
  fi

  #Kiểm tra lệnh EXIT để thoát chương trình
  if [ "$input" = "EXIT" ]; then
    break
  fi

  #Kiểm tra lệnh HIST để hiển thị lịch sử 5 phép tính gần nhất
  if [ "$input" = "HIST" ]; then
    tail -n 5 "$HIST_FILE"
    wait_any_key
    clear_screen
    continue
  fi

  #Phân tích lệnh thành 3 phần: a, op, b
  read -r -a arr <<< "$input"
  if [ "${#arr[@]}" -ne 3 ]; then
    echo "SYNTAX ERROR"
    wait_any_key
    clear_screen
    continue
  fi

  #Gán giá trị a, op, b từ mảng đã phân tích
  a="${arr[0]}"
  op="${arr[1]}"
  b="${arr[2]}"

  #Thay thế ANS bằng giá trị thực tế nếu a hoặc b là ANS
  val_a="$a"
  val_b="$b"
  [ "$a" = "ANS" ] && val_a="$ANS"
  [ "$b" = "ANS" ] && val_b="$ANS"

  #Kiểm tra xem val_a và val_b có phải là số hợp lệ hay không
  if ! is_number "$val_a" || ! is_number "$val_b"; then
    echo "SYNTAX ERROR"
    wait_any_key
    clear_screen
    continue
  fi

  #Thực hiện phép tính dựa trên toán tử và lưu kết quả thô vào biến raw_res
  raw_res=""

  #bc -l 2>/dev/null: sử dụng binary computation với tùy chọn -l thiết lập biến scale và chuyển hướng lỗi sang /dev/null để tránh hiển thị lỗi nếu có
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
  #Nếu raw_res rỗng, có nghĩa là đã xảy ra lỗi trong quá trình tính toán (ví dụ: lỗi cú pháp hoặc lỗi toán học), do đó hiển thị thông báo lỗi và tiếp tục vòng lặp
  if [ -z "$raw_res" ]; then
    echo "SYNTAX ERROR"
    wait_any_key
    clear_screen
    continue
  fi

  #Định dạng kết quả để loại bỏ các số 0 thừa và dấu chấm nếu không còn số nào sau đó, sau đó hiển thị kết quả
  res=$(format_result "$raw_res")
  echo "$res"

  #Lưu phép tính vào lịch sử và giữ lại chỉ 5 phép tính gần nhất
  printf "%s %s %s = %s\n" "$a" "$op" "$b" "$res" >> "$HIST_FILE"
  tail -n 5 "$HIST_FILE" > "$HIST_FILE.tmp" && mv "$HIST_FILE.tmp" "$HIST_FILE"

  #Cập nhật biến ANS với kết quả mới và lưu vào file để sử dụng cho các phép tính tiếp theo
  ANS="$res"
  printf "%s\n" "$ANS" > "$ANS_FILE"

  wait_any_key
  clear_screen
done
