import re
import os
import sys

def generate_cpp_from_header(h_file_path):
    """Parses a single .h file and generates a .cpp file with empty definitions."""
    if not os.path.exists(h_file_path):
        print(f"Error: File '{h_file_path}' not found. Skipping.")
        return

    h_filename = os.path.basename(h_file_path)
    # Replaces .h extension with .cpp
    cpp_file_path = os.path.splitext(h_file_path)[0] + ".cpp"

    with open(h_file_path, 'r') as f:
        content = f.read()

    # 1. Identify the Class Name (if any)
    class_match = re.search(r'class\s+(\w+)', content)
    class_name = class_match.group(1) if class_match else None

    # 2. Identify Functions (Constructors, Destructors, and Methods)
    # Matches: ReturnType (optional) Name(Args)
    func_pattern = re.compile(r'([\w:<>&*]+)?\s*([~\w:]+)\s*\(([^)]*)\)\s*(?:const)?\s*;', re.MULTILINE)
    matches = func_pattern.findall(content)

    if not matches:
        print(f"No function declarations found in '{h_filename}'.")
        return

    with open(cpp_file_path, 'w') as f:
        f.write(f'#include "{h_filename}"\n\n')

        for ret_type, func_name, args in matches:
            ret_type = ret_type.strip()
            func_name = func_name.strip()
            args = " ".join(args.split())

            # Determine if it's a constructor or destructor (usually no return type listed)
            is_ctor_dtor = (class_name and (func_name == class_name or func_name == f"~{class_name}"))

            # Format the signature with ClassName:: prefix if inside a class
            full_func_name = f"{class_name}::{func_name}" if class_name else func_name
            
            if is_ctor_dtor:
                f.write(f"{full_func_name}({args}) {{\n")
            else:
                prefix = f"{ret_type} " if ret_type else ""
                f.write(f"{prefix}{full_func_name}({args}) {{\n")
            
            # Add Body Placeholder
            if ret_type != "void" and not is_ctor_dtor and ret_type != "":
                f.write("    return {}; // TODO: Implement\n")
            else:
                f.write("    // TODO: Implement\n")
            
            f.write("}\n\n")

    print(f"Generated: {cpp_file_path}")

def main():
    if len(sys.argv) < 2:
        print("Usage: python3 script.py <file1.h> <file2.h> ... <fileN.h>")
        print("Example: python3 script.py include/*.h")
        sys.exit(1)

    # Process all arguments provided after the script name
    files_to_process = sys.argv[1:]
    
    for h_file in files_to_process:
        generate_cpp_from_header(h_file)

if __name__ == "__main__":
    main()
