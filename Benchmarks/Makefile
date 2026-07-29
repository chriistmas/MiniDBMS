# Compilador y flags. -O2 porque estamos MIDIENDO tiempos: sin optimizacion
# los numeros no reflejarian un uso real del sistema.
CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -Iinclude \
           -I../DiskManager/include \
           -I../BufferManager/include \
           -I../IndexManager/include \
           -I../QueryEngine/include

DISK_DIR   = ../DiskManager/src
BUFFER_DIR = ../BufferManager/src
INDEX_DIR  = ../IndexManager/src
QUERY_DIR  = ../QueryEngine/src

DISK_SRCS = $(DISK_DIR)/physical_disk.cpp \
            $(DISK_DIR)/page.cpp \
            $(DISK_DIR)/disk_manager.cpp \
            $(DISK_DIR)/record.cpp \
            $(DISK_DIR)/heap_file.cpp \
            $(DISK_DIR)/catalog.cpp \
            $(DISK_DIR)/hash_index.cpp

BUFFER_SRCS = $(BUFFER_DIR)/lru_replacer.cpp \
              $(BUFFER_DIR)/buffer_pool_manager.cpp \
              $(BUFFER_DIR)/buffer_pool_adapter.cpp

INDEX_SRCS = $(INDEX_DIR)/bplus_tree.cpp

QUERY_SRCS = $(QUERY_DIR)/predicate.cpp \
             $(QUERY_DIR)/seq_scan_operator.cpp \
             $(QUERY_DIR)/filter_operator.cpp

COMMON_SRCS = $(DISK_SRCS) $(BUFFER_SRCS) $(INDEX_SRCS) $(QUERY_SRCS)

.PHONY: all run clean

all: bench_buffer_pool bench_index_search

bench_buffer_pool: src/bench_buffer_pool.cpp $(COMMON_SRCS)
	mkdir -p results
	$(CXX) $(CXXFLAGS) -o bench_buffer_pool src/bench_buffer_pool.cpp $(COMMON_SRCS)

bench_index_search: src/bench_index_search.cpp $(COMMON_SRCS)
	mkdir -p results
	$(CXX) $(CXXFLAGS) -o bench_index_search src/bench_index_search.cpp $(COMMON_SRCS)

run: all
	./bench_buffer_pool
	./bench_index_search

clean:
	rm -f bench_buffer_pool bench_index_search *.db
