import os
import xlrd

# name of the spreadsheet
fname = "streams.xls"

# open spreadsheet and get handle to the first sheet
wb = xlrd.open_workbook(fname)
sheet = wb.sheet_by_index(0)

print("Name of the sheet:", sheet.name)
print("Number of Rows:", sheet.nrows)
print("Number of Cols:", sheet.ncols)

idCol = 3 # Column numbering starts with 0

for id in range(1, (sheet.nrows)):
    print(sheet.cell_value(id, idCol))
