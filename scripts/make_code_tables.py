
import bs4

import urllib.request as urlreq
import urllib.error as urlerr
import re
from pathlib import Path
from typing import TextIO, Any

BASE_URL = "https://www.nco.ncep.noaa.gov/pmb/docs/grib2/grib2_doc"
TABLES = ["0.0", "4.1", "4.2", "4.5"]

DESCRIPTION_TRANSLATIONS = {
    '0.0': {
        'Meteorological Products': 'Meteorology',
        'Hydrological Products': 'Hydrology',
        'Land Surface Products': 'Land Surface',
        'Satellite Remote Sensing Products': 'Satellite Remote Sensing',
        'Space Weather Products': 'Space Weather',
        'Oceanographic Products': 'Oceanography',
    },
    '4.5': {
        'Ground or Water Surface' : 'Surface',
        'Cloud Base Level': 'Cloud Base',
        'Level of Cloud Tops': 'Cloud Top',
        'Level of 0o C Isotherm': 'Freezing Level',
        'Level of Adiabatic Condensation Lifted from the Surface': 'Lifted Condensation Level',
        'Level of neutral buoyancy or equilibrium': 'Equilibrium level',
        'Nominal Top of the atmosphere': 'Top of Atmosphere',
        'Isobaric Surface': 'Isobaric Level',
        'Specific Altitude Above Mean Sea Level': 'Height Above MSL',
        'Specified Height Level Above Ground': 'Height AGL',
        'Depth Below Land Surface': 'Depth Below Ground',
    }
}


def tag_clean_strings(elem: bs4.Tag):
    return re.sub(r'[\s]*\(.*\)', '', re.sub(r'[\s]+' , ' ', "".join(elem.strings))).strip().replace('\n', ' ')


def parse_url(url: str, column_header_names: dict[str, str]):
    def parse_table(doc: bs4.Tag):
        columns = None
        table = []

        for row in doc.find_all('tr'):
            if columns is None:
                columns = [column_header_names[tag_clean_strings(t)] for t in row.contents if isinstance(t, bs4.Tag)]
            else:
                row_dict: dict[str, str] = {}

                for col_name, col in zip(columns, row.find_all('td')):
                    row_dict[col_name] = tag_clean_strings(col)
                    if col_name == 'units' and (row_dict[col_name] in ['non-dim', '-', 'Numeric'] or 'table' in row_dict[col_name].lower()):
                        row_dict[col_name] = ""

                if 'reserved' not in row_dict[columns[1]].lower():
                    table.append(row_dict)

        return table

    doc = bs4.BeautifulSoup(urlreq.urlopen(url), 'html.parser')
    column_header = list(column_header_names.keys())[0]

    for table_tag in doc.find_all('table'):
        if table_tag.find('th') is None or tag_clean_strings(table_tag.find('th')) != column_header:
            continue

        return parse_table(table_tag)
    

def parse_url_4_1(url: str):
    column_header_names = {'Category': 'index', 'Description': 'meaning', 'Units': 'units'}
    def parse_table(doc: bs4.Tag):
        discipline = None
        columns = None
        table = []

        for row in doc.find_all('tr'):
            if discipline is None:
                header = tag_clean_strings(row.find('td'))
                match = re.match(r'Product Discipline ([\d]+)', header)
                if match is None:
                    raise ValueError(f"Count not match product discipline in '{header}'")
                
                discipline = match.group(1)
            elif columns is None:
                columns = [column_header_names[tag_clean_strings(t)] for t in row.find_all('th')]
            else:
                row_dict: dict[str, str] = {}

                for col_name, col in zip(columns, row.find_all('td')):
                    row_dict[col_name] = tag_clean_strings(col)

                if 'reserved' not in row_dict['meaning'].lower():
                    table.append(row_dict)
        
        return {discipline: table}


    doc = bs4.BeautifulSoup(urlreq.urlopen(url), 'html.parser')
    column_header = list(column_header_names.keys())[0]

    tables = {}
    for table_tag in doc.find_all('table'):
        if table_tag.find('th') is None or tag_clean_strings(table_tag.find('th')) != column_header:
            continue

        tables.update(parse_table(table_tag))
    
    return tables


def _to_cpp_token(row: dict[str, Any], col: str):
    if col == 'index':
        return f'{row[col]}'
    
    return f'"{row[col].replace('"', '\\"')}"'


def _get_table_entry_type(rows: list[dict]):
    columns = set().union(*[set(t.keys()) for t in rows])

    if 'abbrev' in columns:
        return 'Grib2TableEntryUnitsAbbrev'
    if 'units' in columns:
        return 'Grib2TableEntryUnits'
    return 'Grib2TableEntry'


def _write_table(rows: list[dict], name: str, indent: int, fout: TextIO):
    entry_type = _get_table_entry_type(rows)

    fout.write(f"Grib2Table<{entry_type}>(")
    fout.write("{\n")
    indent_chars = ' ' * indent

    for row in rows:
        fout.write(indent_chars)
        fout.write("    {")
        fout.write(", ".join(_to_cpp_token(row, k) for k in row.keys()))
        fout.write("},\n")

    fout.write(indent_chars)
    fout.write('}, ')
    fout.write(f'"{name}"')
    fout.write(')')


class Table:
    id: str
    name: str
    rows: list[dict]

    def __init__(self, id: str, name: str, rows: list[dict]):
        self.id = id
        self.name = name
        self.rows = rows

    @classmethod
    def from_nco(cls, table_id: str):
        section, num = table_id.split('.')
        table = parse_url(f"{BASE_URL}/grib2_table{section}-{num}.shtml", {'Code Figure': 'index', 'Meaning': 'meaning', '': 'units'})

        for row in table:
            row['meaning'] = DESCRIPTION_TRANSLATIONS[table_id].get(row['meaning'], row['meaning'])

        return cls(table_id, '', table)
    
    def to_cpp(self, fout: TextIO):
        table_var_name = self.id.replace('.', '_')
        entry_type = _get_table_entry_type(self.rows)

        fout.write(f"Grib2Table<{entry_type}> Grib2TableManager::get_table_{table_var_name}() const ")
        fout.write("{\n")
        fout.write("    return ")
        _write_table(self.rows, self.name, 4, fout)
        fout.write(';\n}\n\n')


class Table_4_1(Table):
    id: str
    name: str
    rows: dict[str, list[dict]]

    def __init__(self, name: str, rows: dict[str, list[dict]]):
        self.id = "4.1"
        self.name = name
        self.rows = rows

    @classmethod
    def from_nco(cls):
        tables_4_1 = parse_url_4_1(f"{BASE_URL}/grib2_table4-1.shtml")
        return cls("", tables_4_1)
    
    def to_cpp(self, fout: TextIO):
        table_var_name = self.id.replace('.', '_')
        entry_type = _get_table_entry_type(self.rows['0'])

        fout.write(f"Grib2Table<{entry_type}> Grib2TableManager::get_table_{table_var_name}(unsigned short discipline) const ")
        fout.write("{\n")
        fout.write(f'    std::map<std::string, Grib2Table<{entry_type}>> tables' '({\n')

        for disc, tab in self.rows.items():
            fout.write('        {' f'"{disc}", ')
            _write_table(tab, self.name, 8, fout)
            fout.write('},\n')

        fout.write('    });\n\n')
        fout.write('    return tables.at(std::to_string(discipline));\n')
        fout.write('}\n\n')
    
    def get_disciplines(self):
        return list(self.rows.keys())
    
    def get_table(self, discipline: str):
        return self.rows[discipline]

    
class Table_4_2(Table):
    id: str
    name: str
    rows: dict[str, list[dict]]

    def __init__(self, name: str, rows: dict[str, list[dict]]):
        self.id = "4.2"
        self.name = name
        self.rows = rows

    @classmethod
    def from_nco(cls, table_4_1: Table_4_1):
        column_header_names = {'Number': 'index', 'Parameter': 'meaning', 'Units': 'units', 'Abbrev': 'abbrev'}
        tables: dict[str, list[dict]] = {}

        for disc in table_4_1.get_disciplines():
            for row in table_4_1.get_table(disc):
                category = row['index']
                try:
                    tab_4_2 = parse_url(f"{BASE_URL}/grib2_table4-2-{disc}-{category}.shtml", column_header_names)
                except urlerr.HTTPError:
                    continue

                for row_4_2 in tab_4_2:
                    row_4_2['units'] = row_4_2['units'].rstrip(row_4_2['abbrev']).strip()

                tables[f"{disc}.{category}"] = tab_4_2
        return Table_4_2("", tables)
    
    def to_cpp(self, fout: TextIO):
        table_var_name = self.id.replace('.', '_')
        entry_type = _get_table_entry_type(self.rows['0.0'])

        fout.write(f"Grib2Table<{entry_type}> Grib2TableManager::get_table_{table_var_name}(unsigned short discipline, unsigned short category) const ")
        fout.write("{\n")
        fout.write(f'    std::map<std::string, Grib2Table<{entry_type}>> tables' '({\n')

        for disc_cat, tab in self.rows.items():
            fout.write('        {' f'"{disc_cat}", ')
            _write_table(tab, self.name, 8, fout)
            fout.write('},\n')

        fout.write('    });\n\n')
        fout.write('    return tables.at(std::to_string(discipline) + "." + std::to_string(category));\n')
        fout.write('}\n\n')
    

def parse_remote_tables():
    tables: dict[str, Table] = {}

    for table in TABLES:
        if table == "4.1":
            tables[table] = Table_4_1.from_nco()
        elif table == "4.2":
            tables[table] = Table_4_2.from_nco(tables['4.1'])
        else:
            tables[table] = Table.from_nco(table)

    return tables


def output_cpp(tables: dict[str, Table], fname: Path):
    with open(fname, 'w') as fout:
        fout.write('#ifndef __ZAPHOD_GRIB2_TABLE_DEFS__\n#define __ZAPHOD_GRIB2_TABLE_DEFS__\n\n')
        fout.write('#include <zaphod/table.h>\n\nusing namespace zaphod;\n\n')

        for key, tab in tables.items():
            tab.to_cpp(fout)

        fout.write('#endif\n')


def main():
    cpp_include = Path('include/zaphod')
    cpp_src = Path('src/zaphod')

    tables = parse_remote_tables()
    output_cpp(tables, fname=(cpp_src / 'table_defs.cpp'))
    


if __name__ == "__main__":
    main()