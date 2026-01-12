EXE=power_detect RT9466 uart_test RT7202 RT7202_20251226 BQ27Z746
all: $(EXE)

power_detect: power_detect.c crc.o
	gcc $@.c -o $@ crc.o

RT9466: RT9466.c
	gcc $@.c -o $@

RT7202: RT7202.c crc.o
	gcc $@.c -o $@ crc.o

RT7202_20251226: RT7202_20251226.c crc.o
	gcc $@.c -o $@ crc.o

BQ27Z746: BQ27Z746.c
	gcc $@.c -o $@

uart_test: uart_test.c
	gcc $@.c -o $@

.c.o:
	gcc -c $^

clean:
	rm -f *.o $(EXE)
