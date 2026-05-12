import gdb
import os

# TizenRT build output directory. The path is taken from a GDB variable set in launch.json.
BUILD_OUTPUT_PATH = gdb.convenience_variable("build_output_path").string()

class ElfSymbolLoader(gdb.Breakpoint):
    """
    Automatically loads symbols into GDB when a binary is loaded.
    Sets a breakpoint at binfmt_loadbinary.c:192 to detect when a binary is loaded.
    """
    def __init__(self):
        super(ElfSymbolLoader, self).__init__('binfmt_loadbinary.c:192', gdb.BP_BREAKPOINT, internal=False)
        self.loaded_symbols = []

    def stop(self):
        """
        This method is executed when the breakpoint is hit.
        It reads information from the 'load_attr' and 'bin' variables to load the symbol file.
        """
        try:
            # Get the current stack frame.
            frame = gdb.selected_frame()

            # Find the 'load_attr' local variable.
            load_attr_var = frame.read_var('load_attr')
            if not load_attr_var:
                print("[AUTOSYM] Could not find 'load_attr' variable.")
                return False # Continue execution.

            # Find the 'bin' local variable.
            bin_var = frame.read_var('bin')
            if not bin_var:
                print("[AUTOSYM] Could not find 'bin' variable.")
                return False # Continue execution.

            # Extract the binary name from the load_attr struct.
            load_attr_val = load_attr_var.dereference()
            elf_name_ptr = load_attr_val['bin_name']
            elf_name = elf_name_ptr.string()

            # Extract the .text section address from the bin struct.
            bin_val = bin_var.dereference()
            text_addr = bin_val['sections'][0] # sections[BIN_TEXT] == sections[0]

            if not elf_name or text_addr == 0:
                print("[AUTOSYM] ELF name or .text address not available.")
                return False # Continue execution.

            # Create the symbol file path on the host. Use the file with '_dbg' suffix which includes debug symbols.
            host_elf_path = os.path.join(BUILD_OUTPUT_PATH, elf_name + '_dbg')

            if not os.path.exists(host_elf_path):
                print(f"[AUTOSYM] Symbol file not found on host: {host_elf_path}")
                return False # Continue execution.

            # Create and execute the GDB command to load symbols.
            cmd = f"add-symbol-file {host_elf_path} {text_addr}"
            print(f"[AUTOSYM] Executing: {cmd}")
            gdb.execute(cmd)
            self.loaded_symbols.append(host_elf_path)

        except gdb.error as e:
            print(f"[AUTOSYM] GDB Error: {e}")
        except Exception as e:
            print(f"[AUTOSYM] An unexpected error occurred: {e}")

        # Do not stop at the breakpoint, continue execution immediately.
        return False

class SymbolCleaner(gdb.Breakpoint):
    """
    Initializes the loaded symbols when the system reboots.
    Sets a breakpoint at '__start' to detect a reboot.
    """
    def __init__(self, symbol_loader):
        super(SymbolCleaner, self).__init__('__start', gdb.BP_BREAKPOINT, internal=False)
        self.symbol_loader = symbol_loader

    def stop(self):
        """
        This method is executed when the breakpoint is hit.
        It removes all symbols loaded by ElfSymbolLoader.
        """
        print("[AUTOSYM] System reboot detected. Clearing loaded symbols.")
        for symbol_file in self.symbol_loader.loaded_symbols:
            try:
                cmd = f"remove-symbol-file {symbol_file}"
                print(f"[AUTOSYM] Executing: {cmd}")
                gdb.execute(cmd)
            except gdb.error as e:
                print(f"[AUTOSYM] GDB Error while removing symbol file: {e}")
        self.symbol_loader.loaded_symbols.clear()
        # Do not stop at the breakpoint, continue execution immediately.
        return False

def load_existing_symbols(symbol_loader):
    """
    Load symbols for binaries that are already loaded when GDB attaches.
    This reads the binary table from TizenRT's binary manager to find loaded binaries.
    """
    import re
    print("[AUTOSYM] Checking for already-loaded binaries...")
    
    try:
        common_loaded = False
        user_bin_count = 0
        
        # Get the number of loaded user binaries
        result = gdb.execute("print g_bin_count", to_string=True)
        # Parse the result, e.g., "$1 = 2"
        match = re.search(r'=\s*(\d+)', result)
        if not match:
            print("[AUTOSYM] Could not parse g_bin_count")
            return
        bin_count = int(match.group(1))
        
        # Iterate through the binary table (index 0 is common library, 1..bin_count are user apps)
        for bin_idx in range(bin_count + 1):
            try:
                # Get the binary name
                result = gdb.execute(f"print bin_table[{bin_idx}].load_attr.bin_name", to_string=True)
                # Parse the result - extract only printable ASCII characters (alphanumeric, underscore, hyphen)
                # GDB may show nulls as \000 or actual null chars, so we filter them out
                match = re.search(r'"([a-zA-Z0-9_-]+)', result)
                if not match:
                    print(f"[AUTOSYM] Could not parse binary name for index {bin_idx}")
                    continue
                bin_name = match.group(1)
                
                # Get the text section address from binp->sections[BIN_TEXT]
                # First check if binp is valid
                result = gdb.execute(f"print bin_table[{bin_idx}].binp", to_string=True)
                if "= 0x0" in result:
                    print(f"[AUTOSYM] Binary '{bin_name}' has no binp (not loaded)")
                    continue
                
                # Get the text section address (sections[0] = BIN_TEXT)
                result = gdb.execute(f"print bin_table[{bin_idx}].binp->sections[0]", to_string=True)
                match = re.search(r'=\s*(0x[0-9a-fA-F]+|\d+)', result)
                if not match:
                    print(f"[AUTOSYM] Could not parse text address for '{bin_name}'")
                    continue
                text_addr = match.group(1)
                
                # Create the symbol file path
                host_elf_path = os.path.join(BUILD_OUTPUT_PATH, bin_name + '_dbg')
                
                if not os.path.exists(host_elf_path):
                    print(f"[AUTOSYM] Symbol file not found on host: {host_elf_path}")
                    continue
                
                # Load the symbols
                cmd = f"add-symbol-file {host_elf_path} {text_addr}"
                print(f"[AUTOSYM] Loading symbols for '{bin_name}': {cmd}")
                gdb.execute(cmd)
                symbol_loader.loaded_symbols.append(host_elf_path)
                
                # Track what was loaded
                if bin_idx == 0:
                    common_loaded = True
                else:
                    user_bin_count += 1
                
            except gdb.error as e:
                print(f"[AUTOSYM] GDB Error processing binary index {bin_idx}: {e}")
            except Exception as e:
                print(f"[AUTOSYM] Error processing binary index {bin_idx}: {e}")
        
        # Print summary of loaded binaries
        if common_loaded and user_bin_count > 0:
            print(f"[AUTOSYM] Loaded: common binary + {user_bin_count} user binaries")
        elif common_loaded:
            print(f"[AUTOSYM] Loaded: common binary only")
        elif user_bin_count > 0:
            print(f"[AUTOSYM] Loaded: {user_bin_count} user binaries (no common binary)")
        else:
            print(f"[AUTOSYM] No binaries loaded")
                
    except gdb.error as e:
        print(f"[AUTOSYM] GDB Error: {e}")
    except Exception as e:
        print(f"[AUTOSYM] Error loading existing symbols: {e}")

# Instantiate breakpoint handlers when the script is sourced.
elf_loader = ElfSymbolLoader()
SymbolCleaner(elf_loader)

print("[AUTOSYM] ELF symbol loader and cleaner scripts loaded.")
print("[AUTOSYM] Breakpoint for loading symbols set at binfmt_loadbinary.c:192.")
print("[AUTOSYM] Breakpoint for cleaning symbols set at __start.")

# Load symbols for already-loaded binaries
load_existing_symbols(elf_loader)