import os
import sys
import subprocess
import json
import yaml
import stat

NETPLAN_DIR = "/etc/netplan"
NETPLAN_BIN = "/usr/sbin/netplan"
NETPLAN_YAML_FILENAME = "50-cloud-init.yaml"

CONFIG_FILE_TEMPLATE = """
network:
  version: 2
  ethernets:
    {interface}:
      dhcp4: {dhcp4}
      dhcp6: {dhcp6}
{addresses}
{routes}
"""

def format_routes_yaml(dhcp4, dhcp6, gateway4, gateway6):
    formatted_routes = []
    if not dhcp4 and gateway4:
        formatted_routes.append(f"        - to: 0.0.0.0/0\n          via: {gateway4}")
    if not dhcp6 and gateway6:
        formatted_routes.append(f"        - to: ::/0\n          via: {gateway6}")
    return "      routes:\n" + "\n".join(formatted_routes) if formatted_routes else ""

def format_addresses_yaml(dhcp4, dhcp6, address4, address6):
    formatted_addresses = []
    if not dhcp4 and address4:
        formatted_addresses.append(f"        - {address4}")
    if not dhcp6 and address6:
        formatted_addresses.append(f"        - {address6}")
    return "      addresses:\n" + "\n".join(formatted_addresses) if formatted_addresses else ""

def generate_netplan_yaml_content(interface, dhcp4, dhcp6, address4, address6, gateway4, gateway6):
    # Conditionally format addresses and routes based on DHCP settings
    addresses = format_addresses_yaml(dhcp4, dhcp6, address4, address6)
    routes = format_routes_yaml(dhcp4, dhcp6, gateway4, gateway6)

    # Substitute the template placeholders
    content = CONFIG_FILE_TEMPLATE.format(
        interface=interface,
        dhcp4=str(dhcp4).lower(),
        dhcp6=str(dhcp6).lower(),
        addresses=addresses,
        routes=routes
    )

    return content

def remove_netplan_file(filename):
    if filename:
        file_path = os.path.join(NETPLAN_DIR, filename)
        try:
            os.remove(file_path)
            print(f"Removed invalid file: {file_path}")
        except Exception as e:
            print(f"Failed to remove file {file_path}: {e}")

def backup_netplan_file(filename):
    backup_filename = f"{filename}.bak"
    try:
        os.rename(os.path.join(NETPLAN_DIR, filename), os.path.join(NETPLAN_DIR, backup_filename))
        print(f"Backup created: {backup_filename}")
        return backup_filename
    except Exception as e:
        print(f"Failed to create backup for {filename}: {e}")
        return None

def restore_backup_file(backup_filename):
    if backup_filename:
        original_filename = backup_filename.rsplit(".bak", 1)[0]
        try:
            os.rename(os.path.join(NETPLAN_DIR, backup_filename), os.path.join(NETPLAN_DIR, original_filename))
            print(f"Restored backup: {original_filename}")
            return original_filename
        except Exception as e:
            print(f"Failed to restore backup {backup_filename}: {e}")
            return None
    return None

def write_netplan_yaml_file(content):
    # Creates new or truncate an existed YAML file and write content into it.
    filename = f"{NETPLAN_DIR}/{NETPLAN_YAML_FILENAME}"
    with open(filename, "w") as yaml_file:
        yaml_file.write(content)

    # Restrict permissions: readable and writable only by the owner (0600)
    os.chmod(filename, stat.S_IRUSR | stat.S_IWUSR)

    return filename

def netplan_generate():
    try:
        # Run the 'netplan generate' command and capture both stdout and stderr
        subprocess.run(["netplan", "generate"], capture_output=True, text=True, check=True)
        return True
    except subprocess.CalledProcessError as e:
        # If there was an error, print to stderr
        sys.stderr.write(e.stderr)
        return False

def verify_netplan_config(interface, dhcp4, dhcp6, address4, address6, gateway4, gateway6):
    # Backup the existing YAML file if present
    backup_filename = backup_netplan_file(NETPLAN_YAML_FILENAME)

    # Generate new YAML content
    content = generate_netplan_yaml_content(interface, dhcp4, dhcp6, address4, address6, gateway4, gateway6)

    # Write the new YAML file (overwriting the old one)
    write_netplan_yaml_file(content)

    # Test new configuration
    succeeded = netplan_generate()
    if not succeeded:
        # If failed, remove the new file and restore the backup
        remove_netplan_file(NETPLAN_YAML_FILENAME)
        if backup_filename:
            restore_backup_file(backup_filename)

    return succeeded

def netplan_apply():
    try:
        # Run the 'netplan apply' command and capture both stdout and stderr
        subprocess.run(["netplan", "apply"], capture_output=True, text=True, check=True)
        return True
    except subprocess.CalledProcessError as e:
        # If there was an error, print to stderr
        sys.stderr.write(e.stderr)
        return False

def apply_or_restore_netplan_config():
    # Try to apply the current configuration
    if not netplan_apply():
        print("New netplan config failed to apply.")

        # If apply fails, attempt to restore previous from the backup
        backup_filename = f"{NETPLAN_YAML_FILENAME}.bak"

        if os.path.exists(os.path.join(NETPLAN_DIR, backup_filename)):
            remove_netplan_file(NETPLAN_YAML_FILENAME)
            restored_filename = restore_backup_file(backup_filename)

            # Revalidate the restored configuration
            if restored_filename and netplan_apply():
                print("Netplan config restored from backup successfully.")
                return
        print("No valid configuration restored.")
    else:
        print("New netplan config applied successfully.")

def parse_netplan_yaml_file(interface):
    filename = os.path.join(NETPLAN_DIR, NETPLAN_YAML_FILENAME)
    if not os.path.exists(filename):
        print("Error: Netplan configuration file not found.")
        return None

    try:
        with open(filename, "r") as yaml_file:
            config = yaml.safe_load(yaml_file)

        ethernet_config = config.get("network", {}).get("ethernets", {}).get(interface, None)
        if not ethernet_config:
            print(f"Error: Interface '{interface}' not found in the configuration.")
            return None

        dhcp4 = ethernet_config.get("dhcp4", False)
        dhcp6 = ethernet_config.get("dhcp6", False)

        # Extract the first IPv4 and IPv6 addresses
        address4 = next((addr for addr in ethernet_config.get("addresses", []) if ":" not in addr), "")
        address6 = next((addr for addr in ethernet_config.get("addresses", []) if ":" in addr), "")

        routes = ethernet_config.get("routes", [])
        gateway4 = ""
        gateway6 = ""
        for route in routes:
            if route.get("to") == "0.0.0.0/0":
                gateway4 = route.get("via")
            elif route.get("to") == "::/0":
                gateway6 = route.get("via")

        return {
            "__type": "Result",
            "dhcp4": dhcp4,
            "dhcp6": dhcp6,
            "address4": address4,
            "address6": address6,
            "gateway4": gateway4,
            "gateway6": gateway6
        }
    except Exception as e:
        print(f"Error parsing Netplan YAML file: {e}")
        return None

def main():
    if os.geteuid() != 0:
        print("Error: sudo permissions required.")
        sys.exit(1)

    if len(sys.argv) < 2:
        print("Error: Action type required (e.g., 'verify', 'apply').")
        sys.exit(1)

    action = sys.argv[1]

    if action == "verify":
        if len(sys.argv) < 9:
            print("Error: IP configuration parameters required for 'verify' action.")
            sys.exit(1)

        dhcp4 = sys.argv[3].lower() == "true"
        dhcp6 = sys.argv[4].lower() == "true"

        if verify_netplan_config(sys.argv[2], dhcp4, dhcp6, sys.argv[5], sys.argv[6], sys.argv[7], sys.argv[8]) == False:
            sys.exit(1)

    elif action == "apply":
        apply_or_restore_netplan_config()

    elif action == "parse":
        if len(sys.argv) < 3:
            print("Error: Interface name required for 'parse' action.")
            sys.exit(1)

        interface = sys.argv[2]
        result = parse_netplan_yaml_file(interface)
        if result:
            print(json.dumps(result, indent=2))

    else:
        print(f"Error: Unknown action '{action}'. Supported actions are 'verify' and 'apply'.")
        sys.exit(1)

if __name__ == "__main__":
    main()
