#!/usr/bin/python

# Simple helper program to transform e.g. glade files into C source files

import sys

if len(sys.argv) < 3 or len(sys.argv) > 4:
	sys.exit('Usage: ' + sys.argv[0] + ' [input-file] [output-file] <variable-name>')

if sys.argv[1] == '-':
	input = sys.stdin.read()
else:
	input = open(sys.argv[1]).read()

if sys.argv[2] == '-':
	output = sys.stdout
else:
	output = open(sys.argv[2], 'w')

if len(sys.argv) == 4:
	variable = sys.argv[3]
else:
	variable = sys.argv[2] + '_data'

output.write('static const gchar ' + variable + '[] = {')

for char in input[:-1]:
	output.write(hex(ord(char)) + ', ')

output.write(hex(ord(input[-1])) + '};\n')

output.close()
