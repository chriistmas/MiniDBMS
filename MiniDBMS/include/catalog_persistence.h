#ifndef CATALOG_PERSISTENCE_H
#define CATALOG_PERSISTENCE_H

#include "catalog.h"
#include "i_page_store.h"

/*
 * CatalogPersistence
 * -------------------------------------------------------------------------
 * Serializa/reconstruye el Catalog directamente en la Pagina 0 del archivo
 * fisico (la que DiskManager::CreateDatabase ya reserva para esto, pero
 * que ningun modulo anterior llegaba a usar realmente).
 *
 * Formato de la Pagina 0 (PAGE_SIZE bytes):
 *   [int magic]  -> kCatalogMagic si contiene un catalogo valido
 *   [int length] -> bytes que ocupa el texto serializado
 *   [length bytes de texto]  -> Catalog::Serialize()
 *
 * Limitacion conocida (alcance de este proyecto): el catalogo completo
 * debe caber en una sola pagina de 4KB. Es suficiente para un numero
 * moderado de tablas/columnas; escalar a un catalogo multi-pagina queda
 * como trabajo futuro.
 * -------------------------------------------------------------------------
 */
namespace CatalogPersistence {

// Escribe el catalogo en la Pagina 0. Devuelve false si el texto
// serializado no cabe en una pagina.
bool Save(IPageStore& page_store, const Catalog& catalog);

// Intenta leer un catalogo valido desde la Pagina 0. Devuelve false si la
// pagina 0 no contiene un catalogo reconocible (p.ej. base de datos recien
// creada, cuya Pagina 0 esta vacia/en ceros).
bool Load(IPageStore& page_store, Catalog& out_catalog);

} // namespace CatalogPersistence

#endif // CATALOG_PERSISTENCE_H
