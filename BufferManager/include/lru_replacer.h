#ifndef LRU_REPLACER_H
#define LRU_REPLACER_H

#include <list>
#include <unordered_map>
#include <mutex>

/*
 * LRUReplacer
 * -------------------------------------------------------------------------
 * Politica de reemplazo "Least Recently Used" para el Buffer Pool.
 *
 * Solo lleva registro de los FRAMES que estan "unpinned" (pin_count == 0),
 * es decir, candidatos a ser desalojados. El BufferPoolManager es quien
 * decide cuando un frame entra (Unpin) o sale (Pin / Victim) de esta lista.
 *
 * Implementacion clasica: lista doblemente enlazada (orden de uso) +
 * mapa hash (frame_id -> iterador) para O(1) en Pin/Unpin/Victim.
 * -------------------------------------------------------------------------
 */
class LRUReplacer {
public:
    explicit LRUReplacer(size_t num_frames);
    ~LRUReplacer() = default;

    // Elige el frame menos recientemente usado como victima, lo saca de la
    // lista y lo devuelve por referencia. Retorna false si no hay victimas.
    bool Victim(int* frame_id);

    // El frame ya no es candidato a reemplazo (alguien lo va a usar / pin).
    void Pin(int frame_id);

    // El frame queda disponible para ser reemplazado (pin_count llego a 0).
    void Unpin(int frame_id);

    // Cuantos frames son candidatos a reemplazo en este momento.
    size_t Size() const;

private:
    std::list<int> lru_list_; // frente = menos recientemente usado
    std::unordered_map<int, std::list<int>::iterator> position_;
    size_t capacity_;
    mutable std::mutex latch_;
};

#endif // LRU_REPLACER_H
