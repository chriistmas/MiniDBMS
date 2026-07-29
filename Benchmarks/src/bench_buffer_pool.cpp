// Benchmark 1: Hit ratio del Buffer Pool segun tamano del pool y patron de acceso.
//
// Compara 3 patrones de acceso clasicos en literatura de sistemas de BD:
//   - Secuencial:  recorre las paginas en orden, varias pasadas.
//   - Aleatorio uniforme: cada acceso elige una pagina al azar (sin localidad).
//   - Sesgado 80/20 (hot set): 80% de los accesos caen en el 20% de las
//     paginas ("working set" caliente), simulando localidad tipica de
//     workloads reales (principio de Pareto).
//
// Para cada combinacion (pool_size, patron) se ejecutan N_ACCESSES
// accesos (FetchPage + UnpinPage) y se registra el hit ratio resultante.

#include <iostream>
#include <iomanip>
#include <vector>
#include <random>
#include <fstream>
#include "disk_manager.h"
#include "buffer_pool_manager.h"

namespace {

constexpr int kNumPages = 500;     // tamano del "dataset" (paginas en disco)
constexpr int kNumAccesses = 5000; // accesos por experimento

std::vector<int> GenerateSequentialPattern(int num_pages, int num_accesses) {
    std::vector<int> pattern;
    pattern.reserve(num_accesses);
    for (int i = 0; i < num_accesses; i++) {
        pattern.push_back(i % num_pages);
    }
    return pattern;
}

std::vector<int> GenerateUniformRandomPattern(int num_pages, int num_accesses, unsigned seed) {
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> dist(0, num_pages - 1);
    std::vector<int> pattern;
    pattern.reserve(num_accesses);
    for (int i = 0; i < num_accesses; i++) {
        pattern.push_back(dist(rng));
    }
    return pattern;
}

// 80% de los accesos caen en el 20% "caliente" de las paginas.
std::vector<int> GenerateSkewedPattern(int num_pages, int num_accesses, unsigned seed) {
    std::mt19937 rng(seed);
    int hot_size = std::max(1, num_pages / 5);
    std::uniform_int_distribution<int> hot_dist(0, hot_size - 1);
    std::uniform_int_distribution<int> cold_dist(hot_size, num_pages - 1);
    std::uniform_real_distribution<double> coin(0.0, 1.0);

    std::vector<int> pattern;
    pattern.reserve(num_accesses);
    for (int i = 0; i < num_accesses; i++) {
        if (coin(rng) < 0.8) {
            pattern.push_back(hot_dist(rng));
        } else {
            pattern.push_back(cold_dist(rng));
        }
    }
    return pattern;
}

double RunPattern(DiskManager& disk_manager, size_t pool_size, const std::vector<int>& pattern) {
    BufferPoolManager bpm(disk_manager, pool_size);
    for (int page_id : pattern) {
        bpm.FetchPage(page_id);
        bpm.UnpinPage(page_id, /*is_dirty=*/false);
    }
    long long hits = bpm.GetHitCount();
    long long misses = bpm.GetMissCount();
    return (hits + misses) > 0 ? (100.0 * hits) / (hits + misses) : 0.0;
}

} // namespace

int main() {
    std::cout << "=== Benchmark 1: Hit ratio del Buffer Pool ===\n";
    std::cout << "Dataset: " << kNumPages << " paginas | " << kNumAccesses << " accesos por prueba\n\n";

    DiskManager disk_manager("bench_buffer_pool.db");
    disk_manager.CreateDatabase();
    for (int i = 0; i < kNumPages; i++) disk_manager.AllocatePage();

    std::vector<int> seq_pattern = GenerateSequentialPattern(kNumPages, kNumAccesses);
    std::vector<int> rand_pattern = GenerateUniformRandomPattern(kNumPages, kNumAccesses, 42);
    std::vector<int> skew_pattern = GenerateSkewedPattern(kNumPages, kNumAccesses, 42);

    std::vector<size_t> pool_sizes = {4, 8, 16, 32, 64, 128, 250};

    std::ofstream csv("results/buffer_pool_hit_ratio.csv");
    csv << "pool_size,patron,hit_ratio_pct\n";

    std::cout << std::left << std::setw(12) << "pool_size"
              << std::setw(15) << "secuencial(%)"
              << std::setw(15) << "aleatorio(%)"
              << std::setw(15) << "sesgado_80_20(%)" << "\n";
    std::cout << std::string(57, '-') << "\n";

    for (size_t pool_size : pool_sizes) {
        double seq_ratio = RunPattern(disk_manager, pool_size, seq_pattern);
        double rand_ratio = RunPattern(disk_manager, pool_size, rand_pattern);
        double skew_ratio = RunPattern(disk_manager, pool_size, skew_pattern);

        std::cout << std::left << std::setw(12) << pool_size
                  << std::setw(15) << std::fixed << std::setprecision(2) << seq_ratio
                  << std::setw(15) << rand_ratio
                  << std::setw(15) << skew_ratio << "\n";

        csv << pool_size << ",secuencial," << seq_ratio << "\n";
        csv << pool_size << ",aleatorio," << rand_ratio << "\n";
        csv << pool_size << ",sesgado_80_20," << skew_ratio << "\n";
    }

    csv.close();
    disk_manager.CloseDatabase();
    std::cout << "\nResultados guardados en results/buffer_pool_hit_ratio.csv\n";
    return 0;
}
