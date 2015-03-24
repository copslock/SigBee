#
#
xdr_receiver: xdr_receiver.o
	cc -o $@  $^
xdr_receiver.o: xdr_receiver.c
	cc -o $@ -c -O2 -Wall -I../../include $^
.PHONY: clean
clean:
	rm *.o xdr_receiver
