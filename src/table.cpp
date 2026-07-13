
#include <fstream>
#include <iostream>
#include <memory>

#include <zaphod/table.h>

using namespace zaphod;

std::ostream& zaphod::operator<<(std::ostream& stream, const Grib2TableEntry& entry) {
    stream << "{" << entry.number << ", " << entry.meaning << ", " << entry.units << ", " << (entry.status == OperationalStatus::OPERATIONAL ? "Operational" : "Deprecated") << "}";
    return stream;
}

Grib2TableEntry Grib2Table::get_entry(unsigned int number) const {
    for (auto it = this->entries.begin() ; it != this->entries.end() ; it++) {
        if (it->number == number) return *it;
    }

    throw std::string("Table \"") + this->name + std::string("\" has no entry ") + std::to_string(number);
}

std::ostream& zaphod::operator<<(std::ostream& stream, const Grib2Table& table) {
    stream << "{" << std::endl << "  " << table.name << " (" << (table.type == TableType::CODE ? "Code" : "Flags") << ")" << std::endl;
    for (auto it = table.entries.begin() ; it != table.entries.end(); it++) {
        stream << "  " << *it << std::endl;
    }
    stream << "}";

    return stream;
}

Grib2Table Grib2TableManager::get_table(unsigned char section, unsigned short num) const {
    if (section == 4 && num == 1) {
        throw std::string("Grib2TableManager::get_table() needs a discipline for table 4.1");
    }

    if (section == 4 && num == 2) {
        throw std::string("Grib2TableManager::get_table() needs a discipline and category for table 4.2");
    }

    return this->get_table(section, num, 0, 0);
}

Grib2Table Grib2TableManager::get_table(unsigned char section, unsigned short num, unsigned short discipline) const {
    if (section == 4 && num == 2) {
        throw std::string("Grib2TableManager::get_table() needs a category for table 4.2");
    }

    return this->get_table(section, num, discipline, 0);
}

Grib2Table Grib2TableManager::get_table(unsigned char section, unsigned short num, unsigned short discipline, unsigned short category) const {
    std::string key = std::to_string(section) + "." + std::to_string(num);

    if (section == 4 && (num == 1 || num == 2)) {
        key += "." + std::to_string(discipline);
    }

    if (section == 4 && num == 2) {
        key += "." + std::to_string(category);
    }

    return this->tables.at(key);
}