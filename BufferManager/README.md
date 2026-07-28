# Buffer Pool Manager — Módulo de Memoria Intermedia

Cache de páginas en RAM ubicada entre `HeapFile` / índices y el
`DiskManager`, con política de reemplazo **LRU**.

```
HeapFile / Index
      |  FetchPage / NewPage / UnpinPage / FlushPage
      v
BufferPoolManager  (frames en RAM + LRUReplacer)
      |  ReadPage / WritePage / AllocatePage   (solo en MISS o flush)
      v
DiskManager (.db)
```

## Componentes

- **LRUReplacer**: lista de frames candidatos a reemplazo (`pin_count == 0`),
  ordenada por uso. `Victim()` en O(1), `Pin()`/`Unpin()` en O(1).
- **BufferPoolManager**: administra un `vector<Frame>` de tamaño fijo
  (`pool_size`), un `page_table_` (`page_id -> frame_id`) y una `free_list_`.
  - `FetchPage`: HIT si la página ya está en un frame (no toca disco);
    MISS si no, busca frame libre o pide víctima al replacer, escribe a
    disco la víctima si estaba *dirty*, y trae la página nueva.
  - `UnpinPage(page_id, is_dirty)`: decrementa `pin_count`; en 0, el frame
    vuelve a ser candidato de reemplazo.
  - `NewPage` / `DeletePage`: delegan la asignación/liberación física en
    `DiskManager::AllocatePage` / `DeallocatePage`.
  - `FlushPage` / `FlushAllPages`: sincroniza páginas sucias a disco bajo
    demanda (no en cada operación).
  - Lleva contadores `hit_count_` / `miss_count_` para evidenciar el
    funcionamiento del caché en el informe.
- **BufferPoolAdapter**: implementa `IPageStore` (la misma interfaz que
  usa `DiskManager`) delegando en un `BufferPoolManager`. Permite que
  `HeapFile`, `StaticHashIndex` y el futuro motor de consultas usen el
  Buffer Pool **sin modificar una sola línea de su código**: basta con
  construirlos pasando un `BufferPoolAdapter&` en vez de un
  `DiskManager&`.

## Compilar y ejecutar

```bash
make        # compila buffer_pool_test
make run    # compila (si hace falta) y ejecuta la prueba
make clean  # limpia binarios y .db generados
```

`tests/main.cpp`:
1. Crea un disco de pruebas.
2. Usa un pool de solo **3 frames** para forzar reemplazos LRU de forma
   observable (crea 4 páginas, la 4ª fuerza un desalojo).
3. Verifica que una página recientemente re-consultada (`FetchPage`) no
   es elegida como víctima, y que sigue siendo recuperable tras el
   reemplazo de otro frame.
4. Instancia un `HeapFile` sobre un `BufferPoolAdapter` e inserta/lee
   registros **a través del Buffer Pool** (no directo a disco), y hace
   `FlushAllPages()` para sincronizar.
5. Imprime estadísticas finales (`hit/miss`, tamaño del pool, páginas en
   disco).
