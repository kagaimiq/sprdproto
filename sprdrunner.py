import usb, argparse

ap = argparse.ArgumentParser(description='Run arbitrary code on an Spreadtrum/Unisoc SoC')
ap.add_argument('--dram', action='store_true', help='Ensure that the DRAM will be initialized')
ap.add_argument('address', help='Address where the binary will be loaded to')
ap.add_argument('file', help='File that will be executed')
args = ap.parse_args()

print(args)
