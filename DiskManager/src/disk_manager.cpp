#include "disk_manager.h"
#include <iostream>
#include <cstring>

DiskManager::DiskManager(const std::string& db_filename)
    : db_filename_(db_filename),
      num_pages_(0),
      physical_disk_(2, 4, 8) {}

DiskManager::~DiskManager() {
    CloseDatabase();
}

bool DiskManager::DatabaseExists() const {
    std::ifstream f(db_filename_);
    return f.good();
}

const std::string& DiskManager::GetFilename() const { return db_filename_; }

const PhysicalDisk& DiskManager::GetPhysicalDisk() const { return physical_disk_; }

long long DiskManager::CalculateOffset(int page_id) const {
    // offset = Page_ID * PAGE_SIZE
    return static_cast<long long>(page_id) * PAGE_SIZE;
}

bool DiskManager::CreateDatabase() {
    std::cout << "=====================================================\n";
    std::cout << " Creando disco / archivo de base de datos: " << db_filename_ << "\n";
    std::cout << "=====================================================\n";

    // 1. Simular y mostrar la geometria fisica del disco (superficies/pistas/sectores)
    physical_disk_.BuildGeometry();
    physical_disk_.PrintGeometry();

    int sectors_per_block = physical_disk_.SectorsPerBlock(PAGE_SIZE);
    std::cout << "Cada pagina del DBMS (" << PAGE_SIZE << " bytes) equivale a "
              << sectors_per_block << " sectores fisicos de "
              << Sector::SECTOR_SIZE << " bytes.\n";

    // 2. Crear el archivo binario que representara el disco/base de datos
    std::ofstream new_file(db_filename_, std::ios::binary | std::ios::trunc);
    if (!new_file.is_open()) {
        std::cerr << "Error: no se pudo crear el archivo de base de datos.\n";
        return false;
    }
    new_file.close();

    num_pages_ = 0;
    free_pages_.clear();

    if (!OpenDatabase()) {
        std::cerr << "Error: no se pudo abrir el archivo recien creado.\n";
        return false;
    }

    // 3. Reservar la Pagina 0 para el catalogo de la base de datos
    int catalog_page_id = AllocatePage();
    std::cout << "Pagina 0 (Page ID " << catalog_page_id
              << ") reservada para el catalogo de la base de datos.\n";

    std::cout << "Disco creado correctamente: " << db_filename_ << "\n";
    std::cout << "=====================================================\n";
    return true;
}

bool DiskManager::OpenDatabase() {
    if (db_file_.is_open()) {
        db_file_.close();
    }

    db_file_.open(db_filename_, std::ios::in | std::ios::out | std::ios::binary);
    if (!db_file_.is_open()) {
        return false;
    }

    db_file_.seekg(0, std::ios::end);
    long long size = static_cast<long long>(db_file_.tellg());
    num_pages_ = static_cast<int>(size / PAGE_SIZE);

    // Simplificacion: al abrir, asumimos que todas las paginas existentes
    // estan ocupadas. La administracion de espacio libre real se hace
    // mediante DeallocatePage() durante la ejecucion del programa.
    free_pages_.assign(num_pages_, false);

    return true;
}

void DiskManager::CloseDatabase() {
    if (db_file_.is_open()) {
        db_file_.flush();
        db_file_.close();
    }
}

void DiskManager::ReadPage(int page_id, char* page_data) {
    long long offset = CalculateOffset(page_id);
    db_file_.seekg(offset, std::ios::beg);
    db_file_.read(page_data, PAGE_SIZE);
}

void DiskManager::WritePage(int page_id, const char* page_data) {
    long long offset = CalculateOffset(page_id);
    db_file_.seekp(offset, std::ios::beg);
    db_file_.write(page_data, PAGE_SIZE);
    db_file_.flush();
}

int DiskManager::AllocatePage() {
    // Reutilizar una pagina libre si existe
    for (size_t i = 0; i < free_pages_.size(); i++) {
        if (free_pages_[i]) {
            free_pages_[i] = false;
            return static_cast<int>(i);
        }
    }

    // No hay paginas libres: extender el archivo con una pagina nueva
    int new_page_id = num_pages_;
    num_pages_++;
    free_pages_.push_back(false);

    Page empty_page(new_page_id);
    WritePage(new_page_id, empty_page.GetData());

    return new_page_id;
}

void DiskManager::DeallocatePage(int page_id) {
    if (page_id >= 0 && page_id < static_cast<int>(free_pages_.size())) {
        free_pages_[page_id] = true;
    }
}

int DiskManager::GetNumPages() const { return num_pages_; }
