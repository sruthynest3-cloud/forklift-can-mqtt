CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -Iinclude

all: forklift_sim

forklift_sim: src/sim_main.c src/forklift_fsm.c src/can_codec.c
	$(CC) $(CFLAGS) $^ -o $@ -lm

test:
	$(CC) $(CFLAGS) --coverage tests/test_forklift.c src/forklift_fsm.c src/can_codec.c -o test_runner -lm
	./test_runner
	gcov test_runner-forklift_fsm test_runner-can_codec

lint:
	cppcheck --enable=warning,style,performance --suppress=unusedFunction --error-exitcode=1 src/ include/

clean:
	rm -f forklift_sim test_runner *.gcno *.gcda *.gcov
