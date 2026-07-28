# Query Engine — Módulo de Procesamiento de Consultas (Pipeline)

Motor de consultas de una sola tabla: **Parser básico** + **operadores
físicos que siguen el Modelo Volcano** (`Open`, `Next`, `Close`), armados
sobre `HeapFile`, `BufferPoolManager` e índices (`BPlusTree` /
`StaticHashIndex`) de los módulos anteriores.

```
"SELECT nombre, edad FROM alumnos WHERE edad > 20"
        |  Parser::Parse
        v
   ParsedQuery { columns, table, where }
        |  QueryEngine::BuildPlan  (planner)
        v
   ProjectOperator(nombre, edad)
        |
   FilterOperator(edad > 20)
        |
   SeqScanOperator(alumnos)          <-- via HeapFile::Iterator
```

Si el `WHERE` es una **igualdad sobre una columna con índice registrado**,
el planner cambia el operador hoja por `IndexScanOperator`, que resuelve
la búsqueda con `BPlusTree::Search` o `StaticHashIndex::Search` en vez de
recorrer toda la tabla:

```
"SELECT * FROM alumnos WHERE id = 5"
        v
   ProjectOperator(*)
        |
   IndexScanOperator(id = 5)         <-- via BPlusTree::Search
```

## Componentes

- **Parser** (`parser.h/.cpp`): tokenizer + parser recursivo simple para
  `SELECT <cols|*> FROM <tabla> [WHERE <col> <op> <valor>]`
  (`op` ∈ `= != <> < <= > >=`; valores enteros o `'string'`).
- **Operator** (`operator.h`): interfaz común `Open/Next/Close`.
- **SeqScanOperator**: recorre toda la tabla vía `HeapFile::Iterator`.
- **IndexScanOperator**: recibe cualquier función `Search(key, &rids)`
  (B+Tree o Hash, misma firma) — no está acoplado a una implementación
  concreta de índice.
- **FilterOperator**: aplica un `Predicate` (WHERE) sobre el flujo del hijo.
- **ProjectOperator**: recorta el `Record` a las columnas pedidas en el
  SELECT, usando el esquema del `Catalog` para saber el tipo de cada campo.
- **QueryEngine**: parsea, arma el plan (`BuildPlan`) y lo ejecuta
  imprimiendo resultados; maneja errores de sintaxis, tabla/columna
  inexistente, etc. sin abortar el programa.

## Compilar y ejecutar

```bash
make        # compila query_engine_test
make run
make clean
```

`tests/main.cpp` crea una tabla `alumnos`, construye un `BPlusTree` sobre
`id`, registra tabla e índice en el `QueryEngine`, y ejecuta 7 consultas
distintas (proyección completa, filtro numérico, acceso por índice,
filtro por string sin índice) más 3 casos de error controlados.
