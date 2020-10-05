CFLAGS=-Wall -g

all:ybxsh background.bin
	-mkfifo my_fifo
	@echo "---------------------------------------------"
	@echo " 😃 SUCCEED!!!"
	@echo -e " Make finished: \c"
	@date


ybxsh:ybxsh.o
	$(CC) $(CFLAGS) $^ -o $@

background.bin:background.o
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm *.o ybxsh background.bin my_fifo
	@echo "Clean Finished!!! 😃"
