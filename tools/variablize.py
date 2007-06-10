#!/usr/bin/python

import sys
import getopt

try:
	opts, args = getopt.getopt(sys.argv[1:], 'sl', ['static', 'len'])
except getopt.GetoptError:
	sys.exit('Usage: ' + sys.argv[0] + ' <options> [input-file] [output-file] [variable-name]')

static = False
variable_len = False

for opt, arg in opts:
	if opt in ('-s', '--static'):
		static = True
	elif opt in ('-l', '--len'):
		variable_len = True

if len(args) < 3:
	sys.exit('Usage: ' + sys.argv[0] + ' <options> [input-file] [output-file] [variable-name]')

input = open(args[0]).read()

if args[1] == '-':
	output = sys.stdout
else:
	output = open(args[1], 'w')

variable = args[2]

if static:
	static = 'static '
else:
	static = ''

output.write(static + 'char ' + variable + '[] = {')

for char in input[:-1]:
	output.write(hex(ord(char)) + ', ')

output.write(hex(ord(input[-1])) + '};\n')

if variable_len:
	output.write(static + 'gsize ' + variable + '_len = ' + str(len(input)) + ';\n')

output.close()
