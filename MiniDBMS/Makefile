# Compilador y flags
CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Iinclude \
           -I../DiskManager/include \
           -I../BufferManager/include \
           -I../IndexManager/include \
           -I../QueryEngine/include

DISK_DIR   = ../DiskManager/src
BUFFER_DIR = ../BufferManager/src
INDEX_DIR  = ../IndexManager/src
QUERY_DIR  = ../QueryEngine/src

# Fuentes propias de la capa de integracion
SRCS = src/catalog_persistence.cpp

DISK_SRCS = $(DISK_DIR)/physical_disk.cpp \
            $(DISK_DIR)/page.cpp \
            $(DISK_DIR)/disk_manager.cpp \
            $(DISK_DIR)/record.cpp \
            $(DISK_DIR)/heap_file.cpp \
            $(DISK_DIR)/catalog.cpp \
            $(DISK_DIR)/csv_loader.cpp \
            $(DISK_DIR)/hash_index.cpp

BUFFER_SRCS = $(BUFFER_DIR)/lru_replacer.cpp \
              $(BUFFER_DIR)/buffer_pool_manager.cpp \
              $(BUFFER_DIR)/buffer_pool_adapter.cpp

INDEX_SRCS = $(INDEX_DIR)/bplus_tree.cpp

QUERY_SRCS = $(QUERY_DIR)/parser.cpp \
             $(QUERY_DIR)/predicate.cpp \
             $(QUERY_DIR)/seq_scan_operator.cpp \
             $(QUERY_DIR)/filter_operator.cpp \
             $(QUERY_DIR)/project_operator.cpp \
             $(QUERY_DIR)/index_scan_operator.cpp \
             $(QUERY_DIR)/query_engine.cpp

ALL_SRCS = $(SRCS) $(DISK_SRCS) $(BUFFER_SRCS) $(INDEX_SRCS) $(QUERY_SRCS) src/main.cpp

BIN = minidbms

.PHONY: all run clean

all: $(BIN)

$(BIN): $(ALL_SRCS)
	$(CXX) $(CXXFLAGS) -o $(BIN) $(ALL_SRCS)

run: $(BIN)
	./$(BIN)

clean:
	rm -f $(BIN) *.db
