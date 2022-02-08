import os
import xlrd

# name of the spreadsheet
file_name = "streams.xls"

# open spreadsheet
wb = xlrd.open_workbook(file_name)

# get handle to the first sheet
sheet = wb.sheet_by_index(0)

# get total number of rows and columns
nof_rows = sheet.nrows
nof_cols = sheet.ncols

# the column which contains the name of the fieds
name_column = 4

def print_banner():
    print(80 * '-')
    print(f"Spreadsheet       : {file_name}")
    print(f"Number of Rows    : {nof_rows}")
    print(f"Number of Columns : {nof_cols}")
    print(80 * '-')

def gen_struct():
    name_list = []
    # extract the names of all the fields
    for row in range(1, (nof_rows)):
        name_list.append(sheet.cell_value(row,name_column))
    struct_code = f'struct field ' + '{\n'
    for column in range(0, nof_cols):
        data_type = sheet.cell_value(0, column).split(':')[0]
        field_name = sheet.cell_value(0, column).split(':')[1].strip()
        struct_code += f"    {data_type:<{20}}"
        struct_code += f"{field_name};\n"
    struct_code += '};'
    print(struct_code)

def fillVector():
    for row in range(1, (nof_rows)):
        name_list = []
        for column in range(0, (nof_cols)):
            tmp = sheet.cell_value(row,column)
            if type(tmp) != str:
                name_list.append(int(tmp))
            else:
                name_list.append(tmp)
        print(str(name_list))

def clear_screen():
    if os.name == "posix":
        os.system("clear")
    else:
        os.system("cls")

if __name__ == "__main__":
    clear_screen()
    print_banner()
    gen_struct()
    # fillVector()
