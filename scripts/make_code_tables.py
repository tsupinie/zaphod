
import bs4

import urllib.request as urlreq
import urllib.error as urlerr
import re
from pathlib import Path
from typing import TextIO, Any
import argparse

BASE_URL = "https://www.nco.ncep.noaa.gov/pmb/docs/grib2/grib2_doc"
TABLES = ["0.0", "4.1", "4.2", "4.5"]

DESCRIPTION_TRANSLATIONS = {
    '0.0': {
        'meteorological products': 'meteorology',
        'hydrological products': 'hydrology',
        'land surface products': 'land surface',
        'satellite remote sensing products': 'satellite remote sensing',
        'space weather products': 'space weather',
        'oceanographic products': 'oceanography',
    },
    '4.2.0.2': {
        'σ coordinate vertical velocity': 'sigma coordinate vertical velocity',
        'η coordinate vertical velocity': 'eta coordinate vertical velocity',
    },
    '4.5': {
        'ground or water surface' : 'surface',
        'cloud base level': 'cloud base',
        'level of cloud tops': 'cloud top',
        'level of 0o c isotherm': 'freezing level',
        'level of adiabatic condensation lifted from the surface': 'lifted condensation level',
        'level of neutral buoyancy or equilibrium': 'equilibrium level',
        'nominal top of the atmosphere': 'top of atmosphere',
        'isobaric surface': 'isobaric level',
        'specific altitude above mean sea level': 'height above msl',
        'specified height level above ground': 'height agl',
        'depth below land surface': 'depth below ground',
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

                    if col_name == 'meaning':
                        row_dict[col_name] = row_dict[col_name].lower()

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

                    if col_name == 'meaning':
                        row_dict[col_name] = row_dict[col_name].lower()

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
    
    def check_unique_meanings(self):
        for sub_id, table in self.rows.items():
            meanings: set[str] = set()
            meanings_called_out: set[str] = set()
            for row in table:
                if row['meaning'] in meanings and row['meaning'] not in meanings_called_out:
                    print(f'In table 4.1, subtable {sub_id}: meaning "{row['meaning']}" appears multiple times')
                    meanings_called_out.add(row['meaning'])

                meanings.add(row['meaning'])

    
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

        def trim_end(orig: str, *evict: list[str]):
            ret = orig

            for ev in evict[::-1]:
                if ret.endswith(" " + ev):
                    ret = ret[:-(len(ev) + 1)]
            return ret

        for disc in table_4_1.get_disciplines():
            for row in table_4_1.get_table(disc):
                category = row['index']
                try:
                    tab_4_2 = parse_url(f"{BASE_URL}/grib2_table4-2-{disc}-{category}.shtml", column_header_names)
                except urlerr.HTTPError:
                    continue

                for row_4_2 in tab_4_2:
                    # Sometimes the docs have malformed html (missing </td> tags), and this results in units and
                    #   abbreviations getting tacked on where they shouldn't have. This trims that stuff off.
                    row_4_2['units'] = trim_end(row_4_2['units'], row_4_2['abbrev'])
                    row_4_2['meaning'] = trim_end(row_4_2['meaning'], row_4_2['units'].lower(),
                                                  row_4_2['abbrev'].lower())
                    
                    table_id = f'4.2.{disc}.{category}'
                    row_4_2['meaning'] = DESCRIPTION_TRANSLATIONS.get(table_id, {}).get(row_4_2['meaning'], row_4_2['meaning'])

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

    def check_unique_meanings(self):
        for sub_id, table in self.rows.items():
            meanings: set[str] = set()
            meanings_called_out: set[str] = set()
            for row in table:
                if row['meaning'] in meanings and row['meaning'] not in meanings_called_out:
                    print(f'In table 4.2, subtable {sub_id}: meaning "{row['meaning']}" appears multiple times')
                    meanings_called_out.add(row['meaning'])

                meanings.add(row['meaning'])

    def check_unique_abbrevs(self):
        abbrevs: set[str] = set()
        abbrevs_called_out: set[str] = set()
        for sub_id, table in self.rows.items():
            for row in table:
                if row['meaning'].lower() != 'missing' and row['abbrev'] in abbrevs and row['abbrev'] not in abbrevs_called_out:
                    print(f'In table 4.2, abbreviation "{row['abbrev']}" appears multiple times')
                    abbrevs_called_out.add(row['abbrev'])

                abbrevs.add(row['abbrev'])


def parse_remote_tables(check_uniqueness=False):
    tables: dict[str, Table] = {}

    for table in TABLES:
        if table == "4.1":
            tab = Table_4_1.from_nco()

            if check_uniqueness:
                tab.check_unique_meanings()

            tables[table] = tab
        elif table == "4.2":
            tab = Table_4_2.from_nco(tables['4.1'])

            if check_uniqueness:
                tab.check_unique_meanings()
                tab.check_unique_abbrevs()

            tables[table] = tab
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
    ap = argparse.ArgumentParser()
    ap.add_argument('--check-unique', action='store_true')
    ap.add_argument('--output-path', type=Path, default=Path('src/zaphod'))

    args = ap.parse_args()

    tables = parse_remote_tables(check_uniqueness=args.check_unique)
    output_cpp(tables, fname=(args.output_path / 'table_defs.cpp'))
    


if __name__ == "__main__":
    main()