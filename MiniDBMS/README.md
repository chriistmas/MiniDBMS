# MiniDBMS — Capa de Integración Final

Un único programa que une los cuatro módulos en un sistema real, con
prompt interactivo, en vez de binarios de prueba independientes:

```
DiskManager  →  BufferPoolManager  →  Índices (B+Tree / Hash)  →  QueryEngine
```

## Qué hace

- **Abre o crea** el archivo físico `.db` (`DiskManager::DatabaseExists`).
- Si la base **ya existía**: lee el `Catalog` persistido en la **Página 0**
  (`CatalogPersistence::Load`) y reconstruye un `HeapFile` por tabla con
  **todas** sus páginas (`HeapFile::RestorePageIds`), no solo la primera.
- Levanta un **prompt interactivo** (`minidbms> `) con comandos:

  | Comando | Qué hace |
  |---|---|
  | `\dt` | Lista las tablas del catálogo |
  | `\load <csv> <tabla>` | Carga un CSV como tabla nueva, a través del Buffer Pool |
  | `\index btree <tabla> <col> [orden]` | Crea un índice B+Tree sobre una columna INTEGER |
  | `\index hash <tabla> <col> [cubetas]` | Crea un índice Hash sobre una columna INTEGER |
  | `\stats` | Hits/misses del Buffer Pool, páginas en disco, tablas/índices activos |
  | `\help` | Ayuda |
  | `\quit` / `exit` | Persiste el catálogo y cierra |
  | `SELECT col1,col2\|* FROM tabla [WHERE col op valor]` | Ejecuta la consulta (operadores `= != <> < <= > >=`) |

  El `QueryEngine` usa automáticamente el índice B+Tree/Hash cuando el
  `WHERE` es una igualdad sobre una columna indexada; si no, hace un
  `SeqScan` + `Filter`.

- Al salir (`\quit`): refresca la lista de páginas de cada tabla
  (`HeapFile::GetPageIds()`), persiste el `Catalog` de vuelta a la
  **Página 0**, y hace `FlushAllPages()` del Buffer Pool.

## Persistencia real (Página 0)

`CatalogPersistence` serializa el `Catalog` (`Catalog::Serialize()`) y lo
guarda en la Página 0 con un pequeño encabezado `[magic][longitud]`. Es lo
que permite **cerrar el programa y volver a abrirlo** conservando tablas,
esquema e índices reconstruibles. Limitación conocida: el catálogo
completo debe caber en una sola página de 4KB (suficiente para un número
moderado de tablas/columnas).

## Compilar y ejecutar

```bash
make              # genera el binario 'minidbms'
./minidbms                       # usa minidbms.db por defecto
./minidbms mi_base.db            # o un nombre custom
make clean
```

### Ejemplo de sesión

```
minidbms> \load data/alumnos.csv alumnos
Tabla 'alumnos' cargada a traves del Buffer Pool (1 pagina(s)).

minidbms> SELECT * FROM alumnos
  id | nombre | edad
  ------------------------------------------------
  1 | Jose | 23
  2 | Ana | 20
  3 | Pedro | 22
  4 | Lucia | 21
  (4 fila(s))

minidbms> \index btree alumnos id
Indice B+Tree creado sobre alumnos.id (4 claves, orden 8).

minidbms> SELECT * FROM alumnos WHERE id = 3
  id | nombre | edad
  ------------------------------------------------
  3 | Pedro | 22
  (1 fila(s))

minidbms> \stats
Pool size: 32
Hits: 53  Misses: 0
Paginas totales en disco: 7
...

minidbms> \quit
Catalogo guardado en la Pagina 0.
```

Cerrar el proceso y volver a abrir `minidbms minidbms.db` restaura el
catálogo y las tablas automáticamente (probado con `alumnos.csv` y con
`titanic.csv`, 891 filas en 39 páginas, ambos casos íntegros tras
reabrir).

Hay dos CSV de ejemplo en `data/`: `alumnos.csv` (pequeño, ideal para
demos rápidas) y `titanic.csv` (grande, para evidenciar el manejo
multi-página).
