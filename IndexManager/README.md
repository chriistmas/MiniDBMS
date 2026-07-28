# Index Manager — Módulo de Indexación

Estructuras de indexación mapeadas sobre páginas físicas a través de
`IPageStore` (puede ser `DiskManager` directo o, como en las pruebas de
este módulo, un `BufferPoolManager` mediante `BufferPoolAdapter`).

## Componentes

- **BPlusTree** (`bplus_tree.h/.cpp`, nuevo): árbol B+ para claves
  `INTEGER`. Cada **nodo** (hoja o interno) vive en **una página**,
  reutilizando el mecanismo de *Slotted Page* ya existente (sin tocar
  `Page`/`PageHeader`):
  - Slot 0 de cada página: registro de 1 entero (`1`=hoja, `0`=interno).
  - Hoja: slots 1..k = `Record(key, rid.page_id, rid.slot_id)`;
    `PageHeader.next_page_id` apunta a la siguiente hoja (scans ordenados).
  - Interno: slots 1..k = `Record(key, right_child_page_id)`;
    `next_page_id` se reutiliza como puntero al **hijo más izquierdo**.
  - `Insert`: descenso recursivo; si un nodo supera `max_entries`, se
    divide y la clave separadora se **promueve** al padre (si es la raíz,
    se crea una nueva raíz — el árbol crece hacia arriba).
  - `Search`: enrutamiento por comparación de claves en cada nivel interno
    hasta llegar a la hoja correspondiente.
- **StaticHashIndex** (ya existente en el módulo `DiskManager`, sin
  duplicar código): ahora se reutiliza tal cual porque ya trabaja sobre
  `IPageStore`, por lo que corre igual de bien sobre el Buffer Pool.

## Compilar y ejecutar

```bash
make        # compila index_test
make run
make clean
```

`tests/main.cpp`:
1. Crea disco + `BufferPoolManager` + una tabla (`HeapFile`) de ejemplo.
2. Construye un `BPlusTree` con `max_entries = 4` (fuerza varios splits
   con pocos datos, para que sean observables) y lo imprime hoja por hoja
   en orden, demostrando el encadenamiento.
3. Ejecuta búsquedas puntuales (existentes y no existentes).
4. Reconstruye el `StaticHashIndex` original apuntando al mismo Buffer
   Pool, sin modificar su código.
5. Hace *cross-check*: busca por índice y verifica el registro real en la
   tabla vía `HeapFile::GetRecord`.
