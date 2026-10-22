CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -Iinclude

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

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f src/*.o $(TARGET) rail_system.exe