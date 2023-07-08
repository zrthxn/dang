from argparse import ArgumentParser

dangcli = ArgumentParser(
    prog='Dang',
    description='The Dang Language Compiler',
    epilog='Contact me for any issues')

dangcli.add_argument("filename", help="The name or path of file to compile")

