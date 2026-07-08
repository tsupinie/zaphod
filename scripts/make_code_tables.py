#!/usr/bin/env python

import csv
from pathlib import Path
import argparse
import re

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--flags-fname', type=Path, default=Path('./CodeFlag.txt'))
    ap.add_argument('--output-fname', type=Path, default=Path('./src/grib2_table_defs.h'))

    args = ap.parse_args()

    flags_fname: Path = args.flags_fname
    output_fname: Path = args.output_fname

    with open(flags_fname) as fcsv:
        with open(output_fname, 'w') as fout:
            reader = csv.DictReader(fcsv)
            fout.write('#ifndef __GRIB2_TABLE_DEFS__\n#define __GRIB2_TABLE_DEFS__\n\n')
            fout.write('#include "grib2_table.h"\n\nconst Grib2TableManager g2_tables({\n')


            current_table_id = None
            current_table_name = None
            current_table_type = None

            for row in reader:
                if '-' in row['CodeFlag'] or row['CodeFlag'] == '':
                    continue

                match = re.match('(Code|Flag) table ([\d]+\.[\d]+) - (.+)', row['Title_en'])

                if match is None:
                    print(row['Title_en'])
                    continue

                code_or_flags, table_num, table_title = match.groups()

                if row['SubTitle_en'] != "":
                    match = re.match('Product discipline ([\d]+) - [^,]+(?:, parameter category ([\d]+): .+)?', row['SubTitle_en'])

                    if match is None:
                        print(row['SubTitle_en'])
                        continue

                    sub_discipline, sub_category = match.groups()
                    
                    if sub_category is None:
                        table_num = f'{table_num}.{sub_discipline}'
                    else:
                        table_num = f'{table_num}.{sub_discipline}.{sub_category}'

                row_out = {
                    'number': table_num,
                    'type': code_or_flags,
                    'title': table_title,
                    'codes': row['CodeFlag'],
                    'description': row['MeaningParameterDescription_en'].replace('"', '\\"'),
                    'units': row['UnitComments_en'].replace('"', '\\"'),
                    'status': row['Status'].replace('Operationaal', 'Operational').replace('Opertional', 'Operational')
                }

                if row_out['units'] == '-':
                    row_out['units'] = ''

                if current_table_id is None:
                    fout.write(f'    {{"{row_out["number"]}", Grib2Table({{\n')

                    current_table_id = row_out['number']
                    current_table_name = row_out['title']
                    current_table_type = row_out['type']

                elif current_table_id != row_out['number']:
                    fout.write(f'    }}, "{current_table_name}", TableType::{current_table_type.upper()})}},\n\n    {{"{row_out["number"]}", Grib2Table({{\n')
                    current_table_id = row_out['number']
                    current_table_name = row_out['title']
                    current_table_type = row_out['type']

                fout.write(f'        {{{row_out["codes"]},  "{row_out["description"]}", "{row_out["units"]}", OperationalStatus::{row_out["status"].upper()}}},\n')

            fout.write(f'    }}, "{current_table_name}", TableType::{current_table_type.upper()})}},\n}});\n')
            fout.write('#endif\n')


    

if __name__ == "__main__":
    main()