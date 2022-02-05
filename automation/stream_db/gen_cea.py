import os
import xlrd


# name of the spreadsheet
fname = "StreamDB.xls"


# open spreadsheet and get handle to the first sheet
wb = xlrd.open_workbook(fname)
sheet = wb.sheet_by_index(0)


# aquire total number of rows and columns
numRows = sheet.nrows
numCols = sheet.ncols


# the column which contains the name of the fieds
nameColumn = 4


def printBanner():
    print(80 * '-')
    print(f"Spreadsheet       : {fname}")
    print(f"Number of Rows    : {numRows}")
    print(f"Number of Columns : {numCols}")
    print(80 * '-')


def genStruct():
    nameList = []
    # extract the names of all the fields
    for nRow in range(1, (numRows)):
        nameList.append(sheet.cell_value(nRow,nameColumn))
    structCode = f'struct field ' + '{\n'
    for nCol in range(0, numCols):
        dataType = sheet.cell_value(0, nCol).split(':')[0]
        fieldName = sheet.cell_value(0, nCol).split(':')[1].strip()
        structCode += f"    {dataType:<{20}}"
        structCode += f"{fieldName};\n"
    structCode += '};'
    print(structCode)


def fillVector():
    for nRow in range(1, (numRows)):
        nameList = []
        for nCol in range(0, (numCols)):
            tmp = sheet.cell_value(nRow,nCol)
            if type(tmp) != str:
                nameList.append(int(tmp))
            else:
                nameList.append(tmp)
        print(str(nameList))



def clear_screen():
    if os.name == "posix":
        os.system("clear")
    else:
        os.system("cls")


if __name__ == "__main__":
    clear_screen()
    printBanner()
    genStruct()
    # fillVector()
