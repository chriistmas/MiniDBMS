// Benchmark 2: Tiempo de busqueda puntual (por igualdad de clave) segun N,
// comparando 3 estrategias de acceso:
//   - SeqScanOperator + FilterOperator (recorrido completo de la tabla)
//   - BPlusTree::Search
//   - StaticHashIndex::Search
//
// Para cada tamano de tabla N, se insertan N filas (clave = 0..N-1 en
// orden aleatorio) y se miden M busquedas puntuales de claves elegidas al
// azar, promediando el tiempo por busqueda en microsegundos.

#include <iostream>
#include <iomanip>
#include <vector>
#include <random>
#include <chrono>
#include <fstream>
#include <numeric>
#include <algorithm>
#include "disk_manager.h"
#include "buffer_pool_manager.h"
#include "buffer_pool_adapter.h"
#include "heap_file.h"
#include "hash_index.h"
#include "bplus_tree.h"
#include "seq_scan_operator.h"
#include "filter_operator.h"
#include "predicate.h"

namespace {

constexpr int kNumLookups = 300; // busquedas puntuales medidas por N

double MicrosecondsPerOp(const std::chrono::steady_clock::time_point& start,
                          const std::chrono::steady_clock::time_point& end,
                          int num_ops) {
    auto us = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    return static_cast<double>(us) / num_ops;
}

} // namespace

int main() {
    std::cout << "=== Benchmark 2: SeqScan vs B+Tree vs Hash (busqueda puntual) ===\n";
    std::cout << kNumLookups << " busquedas puntuales medidas por cada N\n\n";

    std::vector<int> sizes = {100, 500, 1000, 5000, 10000, 20000};

    std::ofstream csv("results/index_search_latency.csv");
    csv << "n,seqscan_us,btree_us,hash_us,speedup_btree,speedup_hash\n";

    std::cout << std::left << std::setw(10) << "N"
              << std::setw(15) << "SeqScan(us)"
              << std::setw(15) << "B+Tree(us)"
              << std::setw(15) << "Hash(us)"
              << std::setw(15) << "SpeedUp(B+)"
              << std::setw(15) << "SpeedUp(Hash)" << "\n";
    std::cout << std::string(85, '-') << "\n";

    std::mt19937 rng(123);

    for (int n : sizes) {
        std::string db_name = "bench_index_" + std::to_string(n) + ".db";
        DiskManager disk_manager(db_name);
        disk_manager.CreateDatabase();
        BufferPoolManager bpm(disk_manager, /*pool_size=*/128);
        BufferPoolAdapter adapter(bpm);

        int first_page = adapter.AllocatePage();
        HeapFile table(adapter, first_page);

        std::vector<int> keys(n);
        std::iota(keys.begin(), keys.end(), 0);
        std::shuffle(keys.begin(), keys.end(), rng); // orden de insercion aleatorio

        std::vector<RID> rids(n);
        for (int i = 0; i < n; i++) {
            Record r;
            r.AddInt(keys[i]);
            r.AddString("valor_" + std::to_string(keys[i]));
            rids[keys[i]] = table.InsertRecord(r); // rids[key] = RID de esa clave
        }

        BPlusTree btree(adapter, /*max_entries=*/32);
        StaticHashIndex hash_index(adapter, /*num_buckets=*/std::max(16, n / 20));
        for (int i = 0; i < n; i++) {
            btree.Insert(keys[i], rids[keys[i]]);
            hash_index.Insert(keys[i], rids[keys[i]]);
        }

        std::vector<int> query_keys(kNumLookups);
        std::uniform_int_distribution<int> key_dist(0, n - 1);
        for (int i = 0; i < kNumLookups; i++) query_keys[i] = key_dist(rng);

        // --- SeqScan + Filter ---
        auto t0 = std::chrono::steady_clock::now();
        for (int qk : query_keys) {
            Predicate pred;
            pred.column_index = 0;
            pred.op = ComparisonOp::EQ;
            pred.value.is_int = true;
            pred.value.int_value = qk;
            FilterOperator filter(std::make_unique<SeqScanOperator>(table), pred);
            filter.Open();
            Record out;
            filter.Next(out);
            filter.Close();
        }
        auto t1 = std::chrono::steady_clock::now();
        double seq_us = MicrosecondsPerOp(t0, t1, kNumLookups);

        // --- B+Tree ---
        auto t2 = std::chrono::steady_clock::now();
        for (int qk : query_keys) {
            std::vector<RID> out;
            btree.Search(qk, out);
        }
        auto t3 = std::chrono::steady_clock::now();
        double btree_us = MicrosecondsPerOp(t2, t3, kNumLookups);

        // --- Hash ---
        auto t4 = std::chrono::steady_clock::now();
        for (int qk : query_keys) {
            std::vector<RID> out;
            hash_index.Search(qk, out);
        }
        auto t5 = std::chrono::steady_clock::now();
        double hash_us = MicrosecondsPerOp(t4, t5, kNumLookups);

        double speedup_btree = (btree_us > 0) ? seq_us / btree_us : 0.0;
        double speedup_hash = (hash_us > 0) ? seq_us / hash_us : 0.0;

        std::cout << std::left << std::setw(10) << n
                  << std::setw(15) << std::fixed << std::setprecision(2) << seq_us
                  << std::setw(15) << btree_us
                  << std::setw(15) << hash_us
                  << std::setw(15) << (std::to_string(speedup_btree).substr(0, 5) + "x")
                  << std::setw(15) << (std::to_string(speedup_hash).substr(0, 5) + "x") << "\n";

        csv << n << "," << seq_us << "," << btree_us << "," << hash_us << ","
            << speedup_btree << "," << speedup_hash << "\n";

        disk_manager.CloseDatabase();
    }

    csv.close();
    std::cout << "\nResultados guardados en results/index_search_latency.csv\n";
    return 0;
}
