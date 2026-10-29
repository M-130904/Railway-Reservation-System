
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

# ---------- Google Test ----------
TEST_OBJ_DIR = $(TEST_DIR)/obj
TEST_CORE_OBJS = $(CORE_SRC:src/%.c=$(TEST_OBJ_DIR)/%.o)
GTEST_RPATH = -Wl,-rpath,/usr/local/lib64

# Compile library sources as C (gcc), not C++
$(TEST_OBJ_DIR)/%.o: src/%.c
	mkdir -p $(TEST_OBJ_DIR)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

-include $(TEST_CORE_OBJS:.o=.d)

# Build every test, then run them one after another
gtests: $(addprefix $(TEST_DIR)/,$(GTEST_NAMES))
	@fail=0; for t in $^; do \
		echo "== Running $$t"; ./$$t || fail=1; \
	done; exit $$fail

# General rule: C++ test file + C objects
$(TEST_DIR)/%: gtests/%.cpp $(TEST_CORE_OBJS)
	mkdir -p $(TEST_DIR)
	$(CXX) $(CXXFLAGS) $(GTEST_INCLUDE) $< $(TEST_CORE_OBJS) \
		$(GTEST_LIBDIR) $(GTEST_RPATH) -W1,--allow-multiple-definition $(GTEST_LIBS) -o $@

# One rule for every test: link all core objects; a test's own stubs win
$(TEST_DIR)/%: gtests/%.cpp $(TEST_CORE_OBJS)
	mkdir -p $(TEST_DIR)
	$(CXX) $(CXXFLAGS) $(GTEST_INCLUDE) $< $(TEST_CORE_OBJS) \
		$(GTEST_LIBDIR) $(GTEST_RPATH) -Wl,--allow-multiple-definition $(GTEST_LIBS) -o $@

# One rule for every test
$(TEST_DIR)/%: gtests/%.cpp $(TEST_CORE_OBJS)
	mkdir -p $(TEST_DIR)
	$(CXX) $(CXXFLAGS) $(GTEST_INCLUDE) $< \
		$(filter-out $(addprefix $(TEST_OBJ_DIR)/,$(addsuffix .o,$($*_EXCLUDE))),$(TEST_CORE_OBJS)) \
		$(GTEST_LIBDIR) $(GTEST_RPATH) $(GTEST_LIBS) -o $@

# Cppcheck
cppcheck:
	cppcheck --enable=warning,style,performance,portability \
		--error-exitcode=1 -I include $(SRC)

# MISRA analysis
misra:
	cppcheck --addon=/ucrt64/share/cppcheck/addons/misra.py --enable=style \
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
		objs=""; \
		for o in $(COV_OBJ_DIR)/*.o; do \
			case "$$n:$$o" in \
				test_passenger:*booking.o|test_passenger:*cancellation.o|test_passenger:*search.o|test_passenger:*utils.o) ;; \
				test_schedule:*train.o) ;; \
				*) objs="$$objs $$o" ;; \
			esac; \
		done; \
		$(CXX) $(CXXFLAGS) --coverage $(GTEST_INCLUDE) \
			"$$t" $$objs $(GTEST_LIBDIR) $(GTEST_LIBS) \
			-o "$(COV_TEST_DIR)/$$n"; \
	done
	@set -e; for t in $(COV_TEST_DIR)/test_*; do \
		[ -f "$$t" ] || continue; \
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
