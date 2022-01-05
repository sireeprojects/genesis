from os import readlink

fp = open("input.data", "r")

type_str = fp.readline().split(':')[1]
def_str = fp.readline().split(':')[1]

item_str = fp.readlines()

w = 0
for item in item_str:
    if w < len(item.strip()):
        w = len(item)
w = w-1

# write formatted output
cpp = open("cea_structs.h", "a")

op = f'string to_str({type_str.strip()} t)' \
        + ' {' \
        + '\n' \
        + '\tstring name;\n' \
        + '\tswitch(t)' \
        + ' {\n'
cpp.write(op)

for item in item_str:
    op = f'\t\tcase {item.strip():<{w}} : ' \
        + '{ ' \
        + 'name = ' \
        + '\"' \
        + f'{item.strip():<{w}}' \
        + '\"; break; ' \
        + '}\n'
    cpp.write(op)

op = f'\t\tdefault' \
        + (((6+w)-7)*' ')\
        + ': { ' \
        + f'name = \"{def_str.strip():<{w}}\"; break; ' \
        + '}\n'
cpp.write(op)

op = '\t}\n\treturn cea_trim(name);\n}\n\n\n'
cpp.write(op)

fp.close()
cpp.close()
