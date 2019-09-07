import sys
from intelhex import IntelHex

INPUT_FN = "baromdisp-85.ino.hex"
OUTPUT_FN = "baromdisp-85.ino.withid.hex"

id = int(sys.argv[1])
sleep_time = int(sys.argv[2])
sleep_repeat = int(sys.argv[3])
max_same = int(sys.argv[4])
enable_radioen = int(sys.argv[5])
enable_steady = int(sys.argv[6])

ih = IntelHex(INPUT_FN)

for i in range(0, len(ih)-4):
    if (ih[i] == 0xCE) and (ih[i+1] == 0xFA) and (ih[i+2] == 0xED) and (ih[i+3] == 0xFE):
        print "patching id at index", i, "to value", id
#        ih[i] = id
#        ih[i+1] = 0
#        ih[i+2] = 0
#        ih[i+3] = 0

        assert(ih[i+4] == 0x1F)
        ih[i+4] = id

        assert(ih[i+5] == 9)
        ih[i+5] = sleep_time

        assert(ih[i+6] == 4)
        ih[i+6] = sleep_repeat

        assert(ih[i+7] == 20)
        ih[i+7] = max_same

        assert(ih[i+8] == 0)
        ih[i+8] = enable_radioen

        assert(ih[i+9] == 0)
        ih[i+9] = enable_steady
        
        ih.write_hex_file(OUTPUT_FN)
        sys.exit(0)

print "not found"
sys.exit(-1)
