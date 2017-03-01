# -*- coding: utf-8 -*-
import os.path
import string
import sys


def print_buf(counter, buf):
    buf2 = [('%02x' % ord(i)) for i in buf]
    print '{0}: {1:<39}  {2}'.format(('%07x' % (counter * 16)),
        ' '.join([''.join(buf2[i:i + 2]) for i in range(0, len(buf2), 2)]),
        ''.join([c if c in string.printable[:-5] else '.' for c in buf]))


def process_xxd(file_path):
    with open(file_path, 'r') as f:
        counter = 0
        while True:
            buf = f.read(16)
            if not buf:
                break
            print_buf(counter, buf)
            counter += 1


if __name__ == '__main__':
    if not os.path.exists(sys.argv[1]):
        print >> (sys.stderr, "The file doesn't exist.")
        sys.exit(1)
    process_xxd(sys.argv[1])