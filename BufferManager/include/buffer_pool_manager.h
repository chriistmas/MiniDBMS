#ifndef BUFFER_POOL_MANAGER_H
#define BUFFER_POOL_MANAGER_H

#include <vector>
#include <unordered_map>
#include <list>
#include <mutex>
#include "disk_manager.h"
#include "page.h"
#include "lru_replacer.h"

/*
 * Frame
 * -------------------------------------------------------------------------
 * Slot de RAM que puede contener una pagina del disco. Es la unidad que
 * administra el Buffer Pool (no confundir con Page, que es el contenido
 * logico de 4KB).
 * -------------------------------------------------------------------------
 */
struct Frame {
    Page page;
    int pin_count = 0;
    bool is_dirty = false;
    bool in_use = false; // true si actualmente contiene una pagina valida
};

/*
 * BufferPoolManager
 * -------------------------------------------------------------------------
 * Cache de paginas en memoria RAM ubicada ENTRE la capa logica (HeapFile,
 * indices) y el DiskManager. Objetivo: minimizar accesos a disco.
 *
 *      HeapFile / Index
 *              |  FetchPage / NewPage / UnpinPage / FlushPage
 *              v
 *      BufferPoolManager  --(frames en RAM, política LRU)--
 *              |  ReadPage / WritePage / AllocatePage (solo en miss / flush)
 *              v
 *      DiskManager (.db)
 *
 * Reglas:
 *   - FetchPage: si la pagina ya esta en un frame, incrementa pin_count y
 *     la devuelve sin tocar disco (HIT). Si no esta (MISS), busca un frame
 *     libre o pide una victima al LRUReplacer; si esa victima esta "dirty"
 *     la escribe a disco antes de reutilizar el frame.
 *   - UnpinPage: decrementa pin_count; si llega a 0, el frame pasa a ser
 *     candidato de reemplazo (se agrega al LRUReplacer).
 *   - Un frame con pin_count > 0 JAMAS puede ser elegido como victima.
 *   - Las paginas "sucias" (dirty) solo se escriben a disco al ser
 *     reemplazadas, con FlushPage, o con FlushAllPages (no en cada
 *     operacion), que es la diferencia clave frente a leer/escribir
 *     directo del DiskManager.
 * -------------------------------------------------------------------------
 */
class BufferPoolManager {
public:
    BufferPoolManager(DiskManager& disk_manager, size_t pool_size);
    ~BufferPoolManager();

    // Trae una pagina a RAM (si no estaba ya) e incrementa su pin_count.
    // Devuelve nullptr si no hay frames libres y ninguno es reemplazable.
    Page* FetchPage(int page_id);

    // Libera el pin de una pagina. is_dirty indica si fue modificada.
    bool UnpinPage(int page_id, bool is_dirty);

    // Fuerza la escritura a disco de una pagina especifica (si esta en RAM).
    bool FlushPage(int page_id);

    // Escribe a disco todas las paginas sucias actualmente en el pool.
    void FlushAllPages();

    // Pide una pagina nueva al DiskManager, la trae a un frame (pinned) y
    // devuelve su puntero; page_id_out recibe el Page ID asignado.
    Page* NewPage(int* page_id_out);

    // Elimina una pagina (debe tener pin_count == 0). Libera el frame y la
    // pagina en disco.
    bool DeletePage(int page_id);

    size_t GetPoolSize() const;
    int GetNumPages() const; // delegado al DiskManager subyacente

    // Estadisticas simples, utiles para el informe / evidencias.
    long long GetHitCount() const { return hit_count_; }
    long long GetMissCount() const { return miss_count_; }

private:
    DiskManager& disk_manager_;
    size_t pool_size_;

    std::vector<Frame> frames_;                 // frames_[frame_id]
    std::unordered_map<int, int> page_table_;    // page_id -> frame_id
    std::list<int> free_list_;                   // frames nunca usados / libres
    LRUReplacer replacer_;

    long long hit_count_ = 0;
    long long miss_count_ = 0;

    mutable std::mutex latch_;

    // Busca un frame libre (free_list_) o pide una victima al replacer.
    // Si la victima estaba dirty, la persiste a disco. Devuelve -1 si no
    // hay ningun frame disponible (pool lleno de paginas pineadas).
    int FindAvailableFrame();
};

#endif // BUFFER_POOL_MANAGER_H
