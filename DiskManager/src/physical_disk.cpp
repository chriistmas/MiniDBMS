#include "physical_disk.h"
#include <iostream>

PhysicalDisk::PhysicalDisk(int num_surfaces, int tracks_per_surface, int sectors_per_track)
    : num_surfaces_(num_surfaces),
      tracks_per_surface_(tracks_per_surface),
      sectors_per_track_(sectors_per_track) {}

void PhysicalDisk::BuildGeometry() {
    surfaces_.clear();
    surfaces_.reserve(num_surfaces_);

    for (int s = 0; s < num_surfaces_; s++) {
        Surface surface;
        surface.surface_id = s;
        surface.tracks.reserve(tracks_per_surface_);

        for (int t = 0; t < tracks_per_surface_; t++) {
            Track track;
            track.track_id = t;
            track.sectors.reserve(sectors_per_track_);

            for (int sec = 0; sec < sectors_per_track_; sec++) {
                Sector sector;
                sector.sector_id = sec;
                track.sectors.push_back(sector);
            }
            surface.tracks.push_back(track);
        }
        surfaces_.push_back(surface);
    }
}

void PhysicalDisk::PrintGeometry() const {
    std::cout << "--- Geometria fisica simulada del disco (HDD) ---\n";
    std::cout << "Superficies (platos): " << num_surfaces_ << "\n";
    std::cout << "Pistas por superficie: " << tracks_per_surface_ << "\n";
    std::cout << "Sectores por pista: " << sectors_per_track_ << "\n";
    std::cout << "Tamano de sector: " << Sector::SECTOR_SIZE << " bytes\n";
    std::cout << "Total de sectores simulados: " << TotalSectors() << "\n";
    std::cout << "Capacidad simulada total: " << TotalCapacityBytes() << " bytes\n";

    for (const auto& surface : surfaces_) {
        std::cout << "  Superficie " << surface.surface_id << ":\n";
        for (const auto& track : surface.tracks) {
            std::cout << "    Pista " << track.track_id
                       << " -> " << track.sectors.size() << " sectores\n";
        }
    }
    std::cout << "---------------------------------------------------\n";
}

int PhysicalDisk::TotalSectors() const {
    return num_surfaces_ * tracks_per_surface_ * sectors_per_track_;
}

long long PhysicalDisk::TotalCapacityBytes() const {
    return static_cast<long long>(TotalSectors()) * Sector::SECTOR_SIZE;
}

int PhysicalDisk::SectorsPerBlock(int page_size) const {
    if (Sector::SECTOR_SIZE <= 0) return 0;
    return page_size / Sector::SECTOR_SIZE;
}
