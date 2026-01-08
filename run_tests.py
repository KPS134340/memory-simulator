
import os
import subprocess
import glob

def run_tests():
    test_dir = "tests"
    output_dir = "outputs"
    
    os.makedirs(output_dir, exist_ok=True)
    
    test_files = sorted(glob.glob(os.path.join(test_dir, "*.in")))
    
    for test_file in test_files:
        filename = os.path.basename(test_file)
        name_no_ext = os.path.splitext(filename)[0]
        output_path = os.path.join(output_dir, name_no_ext + ".out")
        
        print(f"Running {name_no_ext}...", end=" ")
        
        with open(test_file, 'r') as infile:
             input_data = infile.read()
             
        process = subprocess.Popen(
            ["./memsim_app"],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )
        
        out, err = process.communicate(input=input_data)
        
        with open(output_path, 'w') as outfile:
            outfile.write(out)
            if err:
                outfile.write("\nSTDERR:\n")
                outfile.write(err)
        
        if process.returncode != 0:
             print("FAILED (Crash)")
        else:
             print("COMPLETED")

if __name__ == "__main__":
    run_tests()
