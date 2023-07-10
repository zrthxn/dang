#!python
from sys import argv

from pydang import dangcli
from pydang import parser
from pydang import codegen

 
if __name__ == "__main__":
    # Parse the arguments to the compiler
    args = dangcli.parse_args(argv[1:])
    
    # Parsing the target file
    tokens = parser.parse(args.filename)
    
    # Codegen for platform
    # codegen.gen_x86
    codegen.gen_amd64(tokens)
    # codegen.gen_arm
    # codegen.gen_aarch
    
    # Call NASM
    # os.system(f"nasm -felf64 {ASM}")
    # os.system(f"ld -o {OUT} {OBJ}")
    # os.system(f"rm {OBJ}")
    # if not args.asm:
    # 	os.system(f"rm {ASM}")