#ifndef __GRIB2_TABLE__
#define __GRIB2_TABLE__

#include <string>
#include <utility>
#include <vector>
#include <map>
#include <iostream>

enum struct OperationalStatus {OPERATIONAL, DEPRECATED, EXPERIMENTAL};
enum struct TableType {CODE, FLAG};

struct Grib2TableEntry {
    unsigned short number;
    std::string meaning;
    std::string units;
    OperationalStatus status;
};

std::ostream& operator<<(std::ostream& stream, const Grib2TableEntry& entry);

struct Grib2Table {
    Grib2Table(std::vector<Grib2TableEntry> entries, std::string name, TableType type) : entries(entries), name(name), type(type) {}

    Grib2TableEntry get_entry(unsigned int number) const;

    friend std::ostream& operator<<(std::ostream&, const Grib2Table&);

    private:
    std::vector<Grib2TableEntry> entries;
    std::string name;
    TableType type;
};

std::ostream& operator<<(std::ostream& stream, const Grib2Table& table);

struct Grib2TableManager {
    Grib2TableManager(const std::map<std::string, Grib2Table>& tables) : tables(tables) {}

    Grib2Table get_table(unsigned char section, unsigned short num) const;
    Grib2Table get_table(unsigned char section, unsigned short num, unsigned short discipline) const;
    Grib2Table get_table(unsigned char section, unsigned short num, unsigned short discipline, unsigned short category) const;

    private:
    std::map<std::string, Grib2Table> tables;
};

#endif