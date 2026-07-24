
#include <fstream>
#include <iostream>
#include <memory>

#include <zaphod/table.h>

using namespace zaphod;

std::ostream& zaphod::operator<<(std::ostream& stream, const Grib2TableEntry& entry) {
    stream << "{" << entry.number << ", " << entry.meaning << "}";
    return stream;
}

std::ostream& zaphod::operator<<(std::ostream& stream, const Grib2TableEntryUnits& entry) {
    stream << "{" << entry.number << ", " << entry.meaning << ", " << entry.units << "}";
    return stream;
}

std::ostream& zaphod::operator<<(std::ostream& stream, const Grib2TableEntryUnitsAbbrev& entry) {
    stream << "{" << entry.number << ", " << entry.meaning << ", " << entry.units << ", " << entry.abbrev << "}";
    return stream;
}

template <typename E>
E Grib2Table<E>::get_entry(unsigned int number) const {
    for (auto it = this->entries.begin() ; it != this->entries.end() ; it++) {
        if (it->number == number) return *it;
    }

    throw std::runtime_error("Table \"" + this->name + "\" has no entry " + std::to_string(number));
}

template <typename E>
E Grib2Table<E>::get_entry_by_meaning(const std::string& meaning) const {
    std::string meaning_lower;
    for (auto it = meaning.begin(); it != meaning.end(); it++) {
        meaning_lower.push_back(std::tolower(*it));
    }

    for (auto it = this->entries.begin() ; it != this->entries.end() ; it++) {
        if (it->meaning == meaning_lower) return *it;
    }

    throw std::runtime_error("Table \"" + this->name + "\" has no entry with meaning \"" + meaning + "\"");
}

template <typename E>
template <typename>
E Grib2Table<E>::get_entry_by_abbrev(const std::string& abbrev) const {
    for (auto it = this->entries.begin() ; it != this->entries.end() ; it++) {
        if (it->abbrev == abbrev) return *it;
    }

    throw std::runtime_error("Table \"" + this->name + "\" has no entry with meaning \"" + abbrev + "\"");
}


template <typename E>
std::ostream& zaphod::operator<<(std::ostream& stream, const Grib2Table<E>& table) {
    stream << "{" << std::endl << "  " << table.name << std::endl;
    for (auto it = table.entries.begin() ; it != table.entries.end(); it++) {
        stream << "  " << *it << std::endl;
    }
    stream << "}";

    return stream;
}

template struct zaphod::Grib2Table<Grib2TableEntry>;
template struct zaphod::Grib2Table<Grib2TableEntryUnits>;
template struct zaphod::Grib2Table<Grib2TableEntryUnitsAbbrev>;
