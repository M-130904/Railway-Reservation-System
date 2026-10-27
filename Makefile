
CC = gcc

CFLAGS = -Wall -Wextra -Wpedantic -std=c99 -g -pthread -Iinclude
LDFLAGS = -pthread

SRC = src/main.c \
      src/logger.c \
      src/auth.c \
      src/admin.c \
      src/passenger.c \
      src/train.c \
      src/schedule.c \
      src/search.c \
      src/booking.c \
      src/cancellation.c \
      src/waitlist.c \
      src/payment.c \
      src/report.c \
      src/file_io.c \
      src/utils.c

OBJ = $(SRC:.c=.o)
TARGET = rail_system

.PHONY: all clean run cppcheck memcheck helgrind misra

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) $(LDFLAGS) -o $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

cppcheck:
	cppcheck --enable=warning,style,performance,portability --error-exitcode=1 -I include $(SRC)

memcheck: $(TARGET)
	valgrind --tool=memcheck --leak-check=full --track-origins=yes ./$(TARGET)

helgrind: $(TARGET)
	valgrind --tool=helgrind --error-exitcode=1 ./$(TARGET)

misra:
	cppcheck --addon=misra.py --enable=style -I include $(SRC)

clean:
	rm -f src/*.o $(TARGET) rail_system.exe
