import sys
from intelhex import IntelHex

INPUT_FN = "baromdisp-85.ino.hex"
OUTPUT_FN = "baromdisp-85.ino.withid.hex"

id = int(sys.argv[1])

ih = IntelHex(INPUT_FN)

for i in range(0, len(ih)-4):
    if (ih[i] == 0x1F) and (ih[i+1] == 0xFA) and (ih[i+2] == 0xED) and (ih[i+3] == 0xFE):
        print "patching id at index", i, "to value", id
        ih[i] = id
        ih[i+1] = 0
        ih[i+2] = 0
        ih[i+3] = 0
        ih.write_hex_file(OUTPUT_FN)
        sys.exit(0)

print "not found"
sys.exit(-1)
