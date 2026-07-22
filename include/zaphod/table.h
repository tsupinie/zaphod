#ifndef __ZAPHOD_GRIB2_TABLE__
#define __ZAPHOD_GRIB2_TABLE__

#include <string>
#include <utility>
#include <vector>
#include <map>
#include <iostream>
#include <type_traits>

namespace zaphod {

struct Grib2TableEntry {
    unsigned short number;
    std::string meaning;
};

std::ostream& operator<<(std::ostream& stream, const Grib2TableEntry& entry);

struct Grib2TableEntryUnits {
    unsigned short number;
    std::string meaning;
    std::string units;
};

std::ostream& operator<<(std::ostream& stream, const Grib2TableEntryUnits& entry);

struct Grib2TableEntryUnitsAbbrev {
    unsigned short number;
    std::string meaning;
    std::string abbrev;
    std::string units;
};

std::ostream& operator<<(std::ostream& stream, const Grib2TableEntryUnitsAbbrev& entry);

template <typename E>
struct Grib2Table {
    Grib2Table(std::vector<E> entries, std::string name) : entries(entries), name(name) {}

    E get_entry(unsigned int number) const;
    E get_entry_by_meaning(const std::string& meaning) const;
    
    template<typename = std::enable_if<std::is_same_v<E, Grib2TableEntryUnitsAbbrev>>>
    E get_entry_by_abbrev(const std::string& abbrev) const;

    friend std::ostream& operator<<(std::ostream&, const Grib2Table<E>&);

    private:
    std::vector<E> entries;
    std::string name;
};

template <typename E>
std::ostream& operator<<(std::ostream& stream, const Grib2Table<E>& table);

struct Grib2TableManager {
    Grib2Table<Grib2TableEntry> get_table_0_0() const;
    Grib2Table<Grib2TableEntry> get_table_4_1(unsigned short discipline) const;
    Grib2Table<Grib2TableEntryUnitsAbbrev> get_table_4_2(unsigned short discipline, unsigned short category) const;
    Grib2Table<Grib2TableEntryUnits> get_table_4_5() const;
};

const Grib2TableManager g2_tables;

}

#endif