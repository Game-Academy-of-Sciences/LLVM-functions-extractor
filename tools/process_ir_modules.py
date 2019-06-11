import os
import argparse

parser = argparse.ArgumentParser()
parser.add_argument('-i', '--input_folder', type=str)
parser.add_argument('-o', '--output_folder', type=str)

args = parser.parse_args()
input_folder = args.input_folder
output_folder = args.output_folder


def clean_modules(file_descriptor):
    content = file_descriptor.readlines()
    prev_delimeter = 0
    pure_module = content

    for i in range(1, len(content)):
        if content[i].startswith(('/src', 'src/', 'w: ', 'ld: ', 'e: ', 'common', 'examples',
             'Picked up _JAVA_OPTIONS', 'nativeprojects', 'app', 'drill', 'PPPShared', 'shared', 'FAILURE')):
            pure_module = content[prev_delimeter:i]
            break
    return pure_module


for filename in os.listdir(input_folder):
   ir_file, ext = os.path.splitext(filename)
   if '.ll' in ext:
       file_path = '{}/{}'.format(input_folder, filename)
       with open(file_path, 'r') as ir_content:
           module = clean_modules(ir_content)
           module_name = "{}/{}".format(output_folder, filename)
           with open(module_name, 'w') as module_file:
               module_file.writelines(module)
               print(module_file)

