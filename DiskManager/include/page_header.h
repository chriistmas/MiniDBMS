#ifndef PAGE_HEADER_H
#define PAGE_HEADER_H

/*
 * PageHeader
 * -------------------------------------------------------------------------
 * Metadatos administrativos almacenados al inicio de cada pagina.
 * No es un registro del usuario: es informacion de control que el
 * Disk Manager / la propia pagina utiliza para saber como esta organizada.
 * -------------------------------------------------------------------------
 */
struct PageHeader {
    int page_id;             // identificador de la pagina
    int record_count;        // cantidad de registros activos (no eliminados)
    int slot_count;          // cantidad total de slots (incluye eliminados)
    int free_space;          // bytes libres disponibles en la pagina
    int free_space_pointer;  // offset donde inicia el area de datos libre
                              // (los registros crecen hacia atras desde el final)
};

#endif // PAGE_HEADER_H
