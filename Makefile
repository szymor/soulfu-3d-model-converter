.PHONY: all clean

PROJECT=s3mc
SRC=main.c

all: $(PROJECT)

$(PROJECT): $(SRC)
	gcc -o $(PROJECT) $(SRC)

clean:
	-rm -f $(PROJECT)
