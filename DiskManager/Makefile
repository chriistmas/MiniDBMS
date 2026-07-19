# Compilador y flags
CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Iinclude

# Directorios
SRC_DIR    = src
TEST_DIR   = tests
TOOLS_DIR  = tools
INCLUDE_DIR = include

# Archivos fuente del modulo (sin main)
SRCS = $(SRC_DIR)/physical_disk.cpp \
       $(SRC_DIR)/page.cpp          \
       $(SRC_DIR)/disk_manager.cpp  \
       $(SRC_DIR)/record.cpp        \
       $(SRC_DIR)/heap_file.cpp     \
       $(SRC_DIR)/catalog.cpp       \
       $(SRC_DIR)/csv_loader.cpp    \
       $(SRC_DIR)/hash_index.cpp

# Ejecutables
TEST_BIN = disk_manager_test
DUMP_BIN = dump_db

# ---------------------------------------------------------------
# Targets
# ---------------------------------------------------------------

.PHONY: all run clean dump

## all: compila el programa de prueba principal
all: $(TEST_BIN)

$(TEST_BIN): $(SRCS) $(TEST_DIR)/main.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^

## dump: compila la herramienta de inspeccion de archivos .db
dump: $(DUMP_BIN)

$(DUMP_BIN): $(SRCS) $(TOOLS_DIR)/dump_db.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^

## run: compila (si hace falta) y ejecuta la prueba
run: $(TEST_BIN)
	./$(TEST_BIN)

## clean: elimina binarios y archivos .db generados
clean:
	rm -f $(TEST_BIN) $(DUMP_BIN) *.db
