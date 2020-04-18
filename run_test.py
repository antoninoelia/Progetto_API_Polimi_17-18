import subprocess
import os
import time

def diff_txt(output_file, my_output_file):
    with open(output_file) as f1, open(my_output_file) as f2:
        for l1, l2 in zip(f1, f2):
            line1 = l1.split()[0]
            line2 = l2.split()[0]
            if line1 != line2:
                return False
            else:
                return True
            

def run_test():
    for input_root, dirs, files in os.walk(test_path + '/input', topdown = True):
        for input_name in files:
            print(os.path.join(input_root, input_name))
            my_output_root = input_root.replace("input", "my_output")
            output_root = input_root.replace("input", "output")
            output_name = input_name.replace("input", "output")
            output_file = os.path.join(output_root, output_name)
            my_output_file = os.path.join(my_output_root, output_name)
            os.system("./mt < " + os.path.join(input_root, input_name) + "> " + my_output_file)
            if(not diff_txt(output_file, my_output_file)):
                print("\n***Test failed!***\n")
                return
            print("\n***Test passed!***\n")


dirname = os.path.abspath(os.getcwd())
dir_source = dirname + '/source/'
test_path = dirname + '/test_case'
print(dirname)
print(dir_source)

os.system("gcc -O2 -o mt " + dir_source + "MT_git.c")
run_test()