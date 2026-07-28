# Compilador y flags
CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Iinclude -I../DiskManager/include

DISK_DIR = ../DiskManager/src

# Fuentes del propio Buffer Manager
SRCS = src/lru_replacer.cpp \
       src/buffer_pool_manager.cpp \
       src/buffer_pool_adapter.cpp

# Fuentes del Disk Manager que el Buffer Manager necesita (capa inferior)
DISK_SRCS = $(DISK_DIR)/physical_disk.cpp \
            $(DISK_DIR)/page.cpp \
            $(DISK_DIR)/disk_manager.cpp \
            $(DISK_DIR)/record.cpp \
            $(DISK_DIR)/heap_file.cpp

TEST_BIN = buffer_pool_test

.PHONY: all run clean

all: $(TEST_BIN)

$(TEST_BIN): $(SRCS) $(DISK_SRCS) tests/main.cpp
	$(CXX) $(CXXFLAGS) -o $(TEST_BIN) $(SRCS) $(DISK_SRCS) tests/main.cpp

run: $(TEST_BIN)
	./$(TEST_BIN)

clean:
	rm -f $(TEST_BIN) *.db
