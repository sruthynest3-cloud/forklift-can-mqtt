CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -Iinclude
GW_LIBS = -lcjson -lpaho-mqtt3c -lm

all: forklift_sim can_mqtt_gateway

forklift_sim: src/sim_main.c src/forklift_fsm.c src/can_codec.c
	$(CC) $(CFLAGS) $^ -o $@ -lm

can_mqtt_gateway: src/gateway_main.c src/mqtt_gateway.c src/can_codec.c src/can_socket.c
	$(CC) $(CFLAGS) $^ -o $@ $(GW_LIBS)

test:
	$(CC) $(CFLAGS) --coverage tests/test_forklift.c src/forklift_fsm.c src/can_codec.c -o test_runner -lm
	./test_runner
	$(CC) $(CFLAGS) --coverage tests/test_gateway.c src/mqtt_gateway.c src/forklift_fsm.c -o test_gateway_runner -lcjson -lm
	./test_gateway_runner
	gcov test_runner-forklift_fsm test_runner-can_codec test_gateway_runner-mqtt_gateway

lint:
	cppcheck --enable=warning,style,performance --suppress=unusedFunction --error-exitcode=1 src/ include/

clean:
	rm -f forklift_sim can_mqtt_gateway test_runner test_gateway_runner *.gcno *.gcda *.gcov