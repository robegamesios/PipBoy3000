from tkinter import filedialog
import tkinter as tk
import re

def convert_h_to_gif():
    root = tk.Tk()
    root.withdraw()

    # Open file dialog for .h file
    input_path = filedialog.askopenfilename(
        title="Select .h file containing GIF data",
        filetypes=[(".h files", "*.h"), ("All files", "*.*")]
    )
    
    if not input_path:
        print("No input file selected")
        return

    # Read the .h file
    with open(input_path, 'r') as f:
        content = f.read()
    
    # Extract the hex values using regex
    hex_values = re.findall(r'0x[0-9a-fA-F]{2}', content)
    if not hex_values:
        print("No hex data found in file")
        return
    
    # Convert hex strings to bytes
    gif_bytes = bytes([int(x, 16) for x in hex_values])

    # Save dialog for output GIF
    save_path = filedialog.asksaveasfilename(
        defaultextension=".gif",
        filetypes=[("GIF files", "*.gif"), ("All files", "*.*")]
    )
    
    if save_path:
        # Write the binary data to file
        with open(save_path, 'wb') as f:
            f.write(gif_bytes)
        print(f"GIF saved as: {save_path}")
    else:
        print("No output location selected")

if __name__ == "__main__":
    convert_h_to_gif()