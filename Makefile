.PHONY: all clean

PROJECT=s3mc
SRC=main.c

all: $(PROJECT)

$(PROJECT): $(SRC)
	gcc -o $(PROJECT) $(SRC) -lm

clean:
	-rm -f $(PROJECT)
