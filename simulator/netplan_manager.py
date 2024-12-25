import os
import sys
import subprocess
import re
import json
import stat

NETPLAN_DIR = "/etc/netplan"
NETPLAN_BIN = "/usr/sbin/netplan"
CONFIG_FILE_TEMPLATE = """
network:
  version: 2
  ethernets:
    {interface}:
      dhcp4: {dhcp4}
      dhcp6: {dhcp6}
      addresses:
{addresses}
      routes:
{routes}
"""

def extract_file_number(filename):
    if filename is None:
        return None

    pattern = r"^(\d+)-.*\.yaml$"

    # Try to match the pattern
    match = re.match(pattern, filename)

    if match:
        # If matched, return the extracted number as an integer
        return int(match.group(1))
    else:
        # If no match, return None
        return None

def find_latest_yaml_file(directory):
    try:
        # List all '.yaml' files in the directory
        files = [f for f in os.listdir(directory)
                 if os.path.isfile(os.path.join(directory, f)) and f.endswith('.yaml')]

        # If the directory is empty or contains no files, return None
        if not files:
            return None

        # Sort the files lexicographically in reverse (descending) order
        files.sort(reverse=True)

        # Return the lexicographically latest file
        return files[0]

    except Exception as e:
        print(f"Failed to find latest yaml file: {e}")
        return None

def remove_netplan_file(filename):
    if filename:
        file_path = os.path.join(NETPLAN_DIR, filename)
        try:
            os.remove(file_path)
            print(f"Removed invalid file: {file_path}")
        except Exception as e:
            print(f"Failed to remove file {file_path}: {e}")

def format_routes_yaml(gateway4, gateway6):
    formatted_routes = []
    if gateway4:
        formatted_routes.append(f"        - to: 0.0.0.0/0\n          via: {gateway4}")
    if gateway6:
        formatted_routes.append(f"        - to: ::/0\n          via: {gateway6}")
    return "\n".join(formatted_routes)

def format_addresses_yaml(addresses4_list, addresses6_list):
    formatted_addresses = []
    for address in addresses4_list:
        formatted_addresses.append(f"        - {address}")
    for address in addresses6_list:
        formatted_addresses.append(f"        - {address}")
    return "\n".join(formatted_addresses)

def generate_netplan_yaml_content(interface, dhcp4, dhcp6, addresses4, addresses6, gateway4, gateway6):
    # Format the lists of addresses into YAML
    addresses = format_addresses_yaml(addresses4, addresses6)

    # Format the routes for YAML
    routes = format_routes_yaml(gateway4, gateway6)

    # Substitute the template placeholders
    content = CONFIG_FILE_TEMPLATE.format(
        interface=interface,
        dhcp4=str(dhcp4).lower(),
        dhcp6=str(dhcp6).lower(),
        addresses=addresses,
        routes=routes
    )

    return content

def write_netplan_yaml_file(interface, content):
    # Create the latest number for yaml file
    number = (extract_file_number(find_latest_yaml_file(NETPLAN_DIR)) or 50) + 1

    # Generate the filename and write content to the YAML file
    filename = f"{NETPLAN_DIR}/{number}-{interface}-config.yaml"
    with open(filename, "w") as yaml_file:
        yaml_file.write(content)

    # Restrict permissions: readable and writable only by the owner (0600)
    os.chmod(filename, stat.S_IRUSR | stat.S_IWUSR)

    return filename

def netplan_generate():
    try:
        # Run the 'netplan generate' command with debug mode and capture both stdout and stderr
        subprocess.run(["netplan", "generate"], capture_output=True, text=True, check=True)
        return True
    except subprocess.CalledProcessError as e:
        # If there was an error, print to stderr
        sys.stderr.write(e.stderr)
        return False

def verify_netplan_config(interface, dhcp4, dhcp6, addresses4, addresses6, gateway4, gateway6):
    content = generate_netplan_yaml_content(interface, dhcp4, dhcp6, addresses4, addresses6, gateway4, gateway6)
    filename = write_netplan_yaml_file(interface, content)

    succedeed = netplan_generate()
    if not succedeed:
        # Remove created file
        remove_netplan_file(filename)

    return succedeed

def apply_netplan_config():
    subprocess.run(["netplan", "apply", "--debug"])

def restore_netplan_config():
    # try run "netplan generate"
    while not netplan_generate():
        # Repeat until generate fails or only one YAML file left in the directory
        yaml_files_left = [f for f in os.listdir(NETPLAN_DIR) if os.path.isfile(os.path.join(NETPLAN_DIR, f)) and f.endswith('.yaml')]
        if len(yaml_files_left) <= 1:
            return

        # Get and remove the latest YAML file which might cause the problem
        filename = find_latest_yaml_file(NETPLAN_DIR)
        if filename is None:
            return
        remove_netplan_file(filename)

def main():
    if os.geteuid() != 0:
        print("Error: sudo permissions required.")
        sys.exit(1)

    if len(sys.argv) < 2:
        print("Error: Action type required (e.g., 'verify', 'apply', 'backup').")
        sys.exit(1)

    action = sys.argv[1]

    if action == "verify":
        if len(sys.argv) < 9:
            print("Error: IP configuration parameters required for 'verify' action.")
            sys.exit(1)

        try:
            # Parse addresses4 and addresses6 as lists
            addresses4 = json.loads(sys.argv[5])
            addresses6 = json.loads(sys.argv[6])

            # Ensure they are lists
            if not isinstance(addresses4, list) or not isinstance(addresses6, list):
                print("Error: Addresses must be JSON arrays.")
                sys.exit(1)
        except json.JSONDecodeError:
            print("Error: Invalid JSON format for addresses4 or addresses6.")
            sys.exit(1)

        if verify_netplan_config(sys.argv[2], sys.argv[3], sys.argv[4], addresses4, addresses6, sys.argv[7], sys.argv[8]) == False:
            sys.exit(1)

    elif action == "apply":
        apply_netplan_config()

    elif action == "backup":
        restore_netplan_config()

    else:
        print(f"Error: Unknown action '{action}'. Supported actions are 'verify', 'apply', and 'backup'.")
        sys.exit(1)

if __name__ == "__main__":
    main()
