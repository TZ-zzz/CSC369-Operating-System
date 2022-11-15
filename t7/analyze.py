import argparse
import fileinput
import operator
import sys
import random

# # Parse command line arguments
# parser = argparse.ArgumentParser(description="Transform reference trace from fastslim to memory simulator input format")
# parser.add_argument("-k", "--keepcode", action="store_true", help="include code pages in trace")
# parser.add_argument("-s", "--simpagesize", type=int, default=16, help="simulated physical page size (smaller than real 4k pagesize)")
# parser.add_argument("tracefile", nargs='?', default='-')
# args = parser.parse_args()

file_path = sys.argv[1]

instructions = {}
data = {}
counts = {'I':0, 'L':0, 'M':0, 'S':0}
zerofills = 0
refills = 0

# Process input trace
with open(file_path, "r") as file:
        for line in file.readlines():
                if line[0] == '=':
                        continue
                tokens = line.strip().split()
                reftype = tokens[0]
                tokens = tokens[1].split(",")
                vaddr = int(tokens[0], 16)
                val = int(tokens[1])

                counts[reftype] = counts[reftype] + 1

                pg = hex(vaddr // 4096)
                if reftype == 'S' or reftype == 'M' or reftype == 'L':
                        if pg in data:
                                data[pg] += 1
                        else:
                                data[pg] = 1
                elif reftype == 'I':
                        if pg in instructions:
                                instructions[pg] += 1
                        else:
                                instructions[pg] = 1
                else:
                        print("Unexpected reftype " + reftype)

        print("Counts:")
        print("  Instructions  {}".format(counts["I"]))
        print("  Loads         {}".format(counts["L"]))
        print("  Stores        {}".format(counts["S"]))
        print("  Modifies      {}".format(counts["M"]))
        print("")
        print("Instructions:")
        i_r =  {key: val for key, val in sorted(instructions.items(), key = lambda ele: ele[1], reverse = True)}
        for v in i_r:
                print("{}000,{}".format(v, i_r[v]))
        print("")
        print("Data:")
        d_r =  {key: val for key, val in sorted(data.items(), key = lambda ele: ele[1], reverse = True)}
        for v in d_r:
                print("{}000,{}".format(v, d_r[v]))
