#ifndef PHYSICAL_DISK_H
#define PHYSICAL_DISK_H

#include <vector>
#include <string>

/*
 * PhysicalDisk
 * -------------------------------------------------------------------------
 * Simula, a nivel conceptual, la geometria fisica de un disco duro (HDD):
 *
 *      Disco fisico
 *          |
 *          v
 *      Superficies (Platters)   -> superficies circulares magnetizadas
 *          |
 *          v
 *      Pistas (Tracks)          -> circulos concentricos dentro de una superficie
 *          |
 *          v
 *      Sectores (Sectors)       -> unidad minima de almacenamiento fisico
 *          |
 *          v
 *      Bloques fisicos          -> agrupacion de sectores contiguos
 *          |
 *          v
 *      Paginas del DBMS         -> unidad logica manejada por el Disk Manager
 *
 * Esta clase NO reemplaza el almacenamiento real: el almacenamiento real
 * (persistencia en disco) lo realiza DiskManager mediante un archivo binario
 * (.db). PhysicalDisk existe unicamente para representar y dejar explicita
 * la jerarquia fisica que justifica por que el DBMS trabaja con bloques y
 * paginas en lugar de sectores individuales.
 * -------------------------------------------------------------------------
 */

struct Sector {
    int sector_id;
    static const int SECTOR_SIZE = 512; // bytes, tamano tipico de un sector fisico
};

struct Track {
    int track_id;
    std::vector<Sector> sectors;
};

// "Plato" o "Superficie" del disco duro
struct Surface {
    int surface_id;
    std::vector<Track> tracks;
};

class PhysicalDisk {
public:
    // Valores por defecto pensados solo para fines demostrativos/didacticos
    PhysicalDisk(int num_surfaces = 2, int tracks_per_surface = 4, int sectors_per_track = 8);

    // Construye la geometria completa: superficies -> pistas -> sectores
    void BuildGeometry();

    // Imprime en consola la jerarquia fisica simulada
    void PrintGeometry() const;

    // Calcula cuantos sectores fisicos totales tiene el disco simulado
    int TotalSectors() const;

    // Capacidad total simulada en bytes (num_sectores * SECTOR_SIZE)
    long long TotalCapacityBytes() const;

    // Cuantos sectores fisicos conforman un bloque/pagina del DBMS
    int SectorsPerBlock(int page_size) const;

    int GetNumSurfaces() const { return num_surfaces_; }
    int GetTracksPerSurface() const { return tracks_per_surface_; }
    int GetSectorsPerTrack() const { return sectors_per_track_; }

private:
    int num_surfaces_;
    int tracks_per_surface_;
    int sectors_per_track_;
    std::vector<Surface> surfaces_;
};

#endif // PHYSICAL_DISK_H
