# Compilador y flags
CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Iinclude \
           -I../DiskManager/include \
           -I../BufferManager/include

DISK_DIR   = ../DiskManager/src
BUFFER_DIR = ../BufferManager/src

# Fuentes propias del modulo de indexacion
SRCS = src/bplus_tree.cpp

# Dependencias de capas inferiores
DISK_SRCS = $(DISK_DIR)/physical_disk.cpp \
            $(DISK_DIR)/page.cpp \
            $(DISK_DIR)/disk_manager.cpp \
            $(DISK_DIR)/record.cpp \
            $(DISK_DIR)/heap_file.cpp \
            $(DISK_DIR)/hash_index.cpp

BUFFER_SRCS = $(BUFFER_DIR)/lru_replacer.cpp \
              $(BUFFER_DIR)/buffer_pool_manager.cpp \
              $(BUFFER_DIR)/buffer_pool_adapter.cpp

TEST_BIN = index_test

.PHONY: all run clean

all: $(TEST_BIN)

$(TEST_BIN): $(SRCS) $(DISK_SRCS) $(BUFFER_SRCS) tests/main.cpp
	$(CXX) $(CXXFLAGS) -o $(TEST_BIN) $(SRCS) $(DISK_SRCS) $(BUFFER_SRCS) tests/main.cpp

run: $(TEST_BIN)
	./$(TEST_BIN)

clean:
	rm -f $(TEST_BIN) *.db
