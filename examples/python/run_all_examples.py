import os
import subprocess

def execute_subdir_py_files(base_dir):
    base_dir = os.path.abspath(base_dir)
    for root, dirs, files in os.walk(base_dir):
        if os.path.abspath(root) == base_dir:
            # Skip the base directory itself
            continue
        for file in files:
            if file.endswith('.py'):
                file_path = os.path.join(root, file)
                print(f"Executing: {file_path}")
                try:
                    subprocess.run(['python', file_path], check=True)
                except subprocess.CalledProcessError as e:
                    print(f"Error executing {file_path}: {e}")
                    return

if __name__ == "__main__":
    execute_subdir_py_files('.')  # or replace '.' with your target base directory