import os
import argparse

parser = argparse.ArgumentParser()
parser.add_argument('-i', '--input_folder', type=str)
parser.add_argument('-o', '--output_folder', type=str)

args = parser.parse_args()
input_folder = args.input_folder
output_folder = args.output_folder


def split_file_by_modules(file_descriptor):
    content = file_descriptor.readlines()
    modules = []
    prev_delimeter = 0
    ir_end = 0
    for i in range(1, len(content)):
        if '; ModuleID =' in content[i]:
            #ir_end = remove_warnings(content[prev_delimeter:i-1])
            modules.append(content[prev_delimeter:i])
            prev_delimeter = i
    modules.append(content[prev_delimeter:])
    return modules

for filename in os.listdir(input_folder):
   ir_file, ext = os.path.splitext(filename)
   if '.ll' in ext:
       file_path = '{}/{}'.format(input_folder, filename)
       with open(file_path, 'r') as ir_content:
           modules = split_file_by_modules(ir_content)
           for i in range(len(modules)):
               module_name = "{}/{}_{}.ll".format(output_folder, ir_file, i)
               with open(module_name, 'w') as module_file:
                   module_file.writelines(modules[i])
                   print(module_file)

