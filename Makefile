
CC = gcc
CXX = g++

CFLAGS = -Wall -Wextra -Wpedantic -std=c99 -g -pthread -Iinclude
CXXFLAGS = -Wall -Wextra -Wpedantic -std=c++17 -g -pthread -Iinclude
LDFLAGS = -pthread

GTEST_INCLUDE = -I/usr/include/llvm-gtest
GTEST_LIBDIR = -L/usr/local/lib64
GTEST_LIBS = -lgtest_main -lgtest -pthread

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

CORE_SRC = $(filter-out src/main.c,$(SRC))
GTEST_SRC = $(wildcard gtests/test_*.cpp)
GTEST_NAMES = $(patsubst gtests/%.cpp,%,$(GTEST_SRC))

TARGET = rail_system
BUILD_DIR = build
APP_OBJ_DIR = $(BUILD_DIR)/app
TEST_DIR = $(BUILD_DIR)/gtest
COV_DIR = $(BUILD_DIR)/coverage
COV_OBJ_DIR = $(COV_DIR)/obj
COV_TEST_DIR = $(COV_DIR)/gtest

.PHONY: all run clean gtests cppcheck misra coverage \
        valgrind memcheck helgrind

# Normal application
all: $(TARGET)

$(TARGET): $(SRC:src/%.c=$(APP_OBJ_DIR)/%.o)
	$(CC) $^ $(LDFLAGS) -o $@

$(APP_OBJ_DIR)/%.o: src/%.c
	mkdir -p $(APP_OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

# Google Test
gtests: $(addprefix $(TEST_DIR)/,$(GTEST_NAMES))
	@set -e; for t in $^; do \
		echo "Running $$t"; ./$$t; \
	done

# General test linking
$(TEST_DIR)/%: gtests/%.cpp $(CORE_SRC)
	mkdir -p $(TEST_DIR)
	$(CXX) $(CXXFLAGS) $(GTEST_INCLUDE) $< $(CORE_SRC) \
		$(GTEST_LIBDIR) $(GTEST_LIBS) -o $@

# Schedule test uses a controlled train stub
$(TEST_DIR)/test_schedule: gtests/test_schedule.cpp \
                            $(filter-out src/train.c,$(CORE_SRC))
	mkdir -p $(TEST_DIR)
	$(CXX) $(CXXFLAGS) $(GTEST_INCLUDE) $< \
		$(filter-out src/train.c,$(CORE_SRC)) \
		$(GTEST_LIBDIR) $(GTEST_LIBS) -o $@

# Cppcheck
cppcheck:
	cppcheck --enable=warning,style,performance,portability \
		--error-exitcode=1 -I include $(SRC)

# MISRA analysis
misra:
	cppcheck --addon=misra.py --enable=style \
		--error-exitcode=1 -I include $(SRC)

# Coverage
coverage:
	rm -rf $(COV_DIR)
	mkdir -p $(COV_OBJ_DIR) $(COV_TEST_DIR)
	@set -e; for s in $(CORE_SRC); do \
		n=$$(basename "$${s%.c}"); \
		$(CC) $(CFLAGS) --coverage -O0 -c "$$s" \
			-o "$(COV_OBJ_DIR)/$$n.o"; \
	done
	@set -e; for t in $(GTEST_SRC); do \
		n=$$(basename "$${t%.cpp}"); \
		objs="$(COV_OBJ_DIR)"/*.o; \
		if [ "$$n" = "test_schedule" ]; then \
			objs=""; \
			for o in $(COV_OBJ_DIR)/*.o; do \
				case "$$o" in */train.o) ;; *) objs="$$objs $$o" ;; esac; \
			done; \
		fi; \
		$(CXX) $(CXXFLAGS) --coverage $(GTEST_INCLUDE) \
			"$$t" $$objs $(GTEST_LIBDIR) $(GTEST_LIBS) \
			-o "$(COV_TEST_DIR)/$$n"; \
	done
	@set -e; for t in $(COV_TEST_DIR)/test_*; do \
		echo "Running $$t"; "$$t"; \
	done
	gcovr --root . --filter 'src/.*' --txt-summary \
		--html-details $(COV_DIR)/index.html $(COV_DIR)
	@echo "HTML coverage report: $(COV_DIR)/index.html"

# Valgrind Memcheck
valgrind: memcheck

memcheck: $(TARGET)
	valgrind --tool=memcheck --leak-check=full \
		--track-origins=yes ./$(TARGET)

# Valgrind Helgrind
helgrind: $(TARGET)
	valgrind --tool=helgrind --error-exitcode=1 ./$(TARGET)

# Clean generated files
clean:
	rm -rf $(BUILD_DIR) $(TARGET) rail_system.exe
	rm -f src/*.o
