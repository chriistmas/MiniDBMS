# Disk Manager (Storage Manager) — Módulo de Disco

Implementación en C++ del componente **Disk Manager** de un DBMS orientado a
disco, según la jerarquía:

```
Disco físico (superficies/pistas/sectores)
        -> Archivo de base de datos (.db)
                -> Páginas (Slotted Pages)
                        -> Registros/Tuplas
```

Este módulo cubre **todo lo relacionado al disco** (incluyendo la
simulación de superficies/platos, pistas y sectores) y **no** implementa
Buffer Pool, frames, políticas de reemplazo (LRU/FIFO/Clock), dirty pages,
pin/unpin ni gestión de RAM — eso corresponde al Buffer Manager, fuera de
este módulo.

## Estructura del proyecto

```
DiskManagerProject/
├── include/                  # Headers (.h)
│   ├── physical_disk.h       # Simulación de superficies/pistas/sectores
│   ├── page_header.h         # Metadatos administrativos de una página
│   ├── page.h                # Página (Slotted Page)
│   ├── disk_manager.h        # Disk Manager (componente principal)
│   ├── record.h              # Registro/Tupla serializable
│   ├── heap_file.h           # Heap File + RID (Page ID, Slot ID)
│   ├── catalog.h             # Catálogo de la base de datos
│   └── csv_loader.h          # Carga de tablas desde CSV
├── src/                       # Implementaciones (.cpp)
│   ├── physical_disk.cpp
│   ├── page.cpp
│   ├── disk_manager.cpp
│   ├── record.cpp
│   ├── heap_file.cpp
│   ├── catalog.cpp
│   └── csv_loader.cpp
├── tests/
│   └── main.cpp               # Programa de prueba end-to-end
├── tools/
│   └── dump_db.cpp            # Herramienta para inspeccionar archivos .db
├── data/
│   ├── alumnos.csv            # CSV de ejemplo para la carga inicial
│   └── titanic.csv            # Otro CSV de ejemplo
├── Makefile                   # Compilación principal (all, run, dump, clean)
├── CMakeLists.txt             # Alternativa con CMake (opcional)
└── README.md
```

## Qué hace cada clase

- **PhysicalDisk**: simula, solo con fines didácticos, la geometría física
  de un HDD (superficies/platos → pistas → sectores) e imprime la
  jerarquía y su relación con el tamaño de página del DBMS. No reemplaza
  el almacenamiento real.
- **DiskManager**: crea/abre el archivo binario `.db`, calcula
  `offset = Page_ID * PAGE_SIZE`, lee/escribe páginas completas, asigna
  (`AllocatePage`) y libera (`DeallocatePage`) páginas, y lleva el control
  de cuántas páginas existen.
- **Page**: implementa una página tipo *Slotted Page* (Page Header + Slot
  Directory + área de datos que crece en sentido inverso). Soporta
  insertar, leer, actualizar y eliminar registros dentro de la página.
- **Record**: registro genérico serializable a bytes (soporta campos
  `INTEGER` y `STRING` de tamaño variable).
- **HeapFile**: colección de páginas de una tabla; inserta registros
  buscando espacio disponible o solicitando páginas nuevas al
  DiskManager; identifica cada registro con un `RID(Page ID, Slot ID)`.
- **Catalog**: guarda el nombre de la base de datos, las tablas, sus
  columnas/tipos y en qué página inicia cada `HeapFile`.
- **CsvLoader**: implementa el flujo `CSV -> Parser -> Esquema -> Páginas
  físicas -> Registros`, infiriendo el tipo de cada columna a partir de
  la primera fila de datos.

## Cómo compilar y ejecutar (Ubuntu / VSCode)

### Opción 1: Makefile (recomendado, más simple)

```bash
make        # compila el programa principal (disk_manager_test)
make run    # compila (si hace falta) y ejecuta la prueba principal
make dump   # compila la herramienta de inspección (dump_db)
make clean  # elimina los binarios y archivos .db generados
```

### Opción 2: CMake

```bash
cd DiskManagerProject
mkdir build && cd build
cmake ..
make
./disk_manager_test
```

### Opción 3: Compilación manual

```bash
g++ -std=c++17 -Iinclude \
    src/*.cpp tests/main.cpp \
    -o disk_manager_test
./disk_manager_test
```

> En VSCode: abre la carpeta `DiskManagerProject`, instala la extensión
> **C/C++** (ms-vscode.cpptools), y usa la terminal integrada (`Ctrl+ñ` o
> `Ctrl+backtick`) para correr `make run`. También puedes usar la
> extensión **CMake Tools** si prefieres el flujo con CMake.

## Qué hace `tests/main.cpp`

1. **Crea el disco**: simula la geometría física (superficies/pistas/
   sectores) y crea el archivo `universidad.db`, reservando la Página 0
   para el catálogo.
2. **Carga interactiva de CSVs**: Muestra un menú listando los archivos
   en `data/`. Puedes cargar múltiples CSVs de forma interactiva (se 
   inferirá el esquema automáticamente) o escribir una ruta personalizada.
3. **Prueba manual de página**: inserta, lee, actualiza y elimina
   registros directamente sobre una página (demuestra el uso de
   `Slot Array` + `RID`).
4. **Prueba de persistencia**: cierra el archivo `.db`, lo vuelve a abrir
   con una nueva instancia de `DiskManager`, e imprime el contenido 
   guardado validando que el almacenamiento en disco realmente funciona.

Al ejecutarlo verás en consola la geometría simulada del disco, el
catálogo, los registros insertados y el resultado de la prueba de persistencia.

## Cómo inspeccionar los datos (dump_db)

Dado que los archivos `.db` generados usan el formato binario interno de nuestro
propio Disk Manager (y no son legibles por SQLite o un editor de texto normal),
hemos creado una herramienta de inspección llamada `dump_db`.

Para usarla:

```bash
make dump
./dump_db universidad.db
```

Esto leerá el archivo página por página y te mostrará un volcado legible:
el encabezado de la página (espacio libre, slots), el directorio de slots y
finalmente todos los registros deserializados, indicando el tipo de cada campo
(INTEGER o STRING) y su valor.

## Notas de diseño

- `PAGE_SIZE = 4096` bytes (definido en `page.h`).
- El offset de una página se calcula como `Page_ID * PAGE_SIZE`
  (ver `DiskManager::CalculateOffset`).
- El RID `(Page ID, Slot ID)` apunta al *slot*, no a la dirección física
  del registro, por lo que un registro puede reubicarse dentro de la
  página sin invalidar referencias externas.
- Este módulo queda explícitamente por **debajo** del Buffer Manager:
  cada llamada a `ReadPage`/`WritePage` va directo a disco (no hay caché
  en memoria), tal como corresponde al límite de responsabilidad del
  Disk Manager.
