#
#
ui_receiver: ui_receiver.o
	cc -o $@  $^
ui_receiver.o: ui_receiver.c
	cc -o $@ -c -O2 -Wall -I../../include $^
.PHONY: clean
clean:
	rm *.o ui_receiver
