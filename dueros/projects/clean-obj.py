import os

print "clean all object before compile"
print "current path:", os.getcwd()
obj_dir = "./build"
all_files = os.listdir(obj_dir)
for a_file in all_files:
    if a_file != "uvision5":
        print "remove file:", os.path.join(obj_dir, a_file)
        os.remove(os.path.join(obj_dir, a_file))

obj_dir = "./build/uvision5"
all_files = os.listdir(obj_dir)
for a_file in all_files:
    file_posix =  os.path.splitext(a_file)[1]
    if file_posix == ".o":
        print "remove file:", os.path.join(obj_dir, a_file)
        os.remove(os.path.join(obj_dir, a_file))

    if file_posix == ".d":
        print "remove file:", os.path.join(obj_dir, a_file)
        os.remove(os.path.join(obj_dir, a_file))

    if file_posix == ".axf":
        print "remove file:", os.path.join(obj_dir, a_file)
        os.remove(os.path.join(obj_dir, a_file))
