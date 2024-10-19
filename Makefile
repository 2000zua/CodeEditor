CC = gcc
FILE = zua.c
OUT = main
FILE_TEXT = example.txt


compile:
	$(CC) $(FILE) -o $(OUT) -Wall -Wextra -pedantic -std=c2x

run:
	touch $(FILE_TEXT)
	./$(OUT) $(FILE_TEXT)

clean:
	rm -rf $(FILE_TEXT)
	rm -rf $(OUT)








