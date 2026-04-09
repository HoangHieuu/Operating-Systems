all: sum_serial sum_multi_thread
sum_serial: sum_serial.c
	gcc -o sum_serial sum_serial.c -lpthread
sum_multi_thread: sum_multi_thread.c
	gcc -o sum_multi_thread sum_multi_thread.c -lpthread
clean:
	rm -f sum_serial sum_multi_thread