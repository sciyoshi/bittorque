import sys

if len(sys.argv) < 3 or len(sys.argv) > 4:
	sys.exit('Usage: ' + sys.argv[0] + ' [input-file] [output-file] <variable-name>')

input = open(sys.argv[1]).read()
output = open(sys.argv[2], 'w')

if len(sys.argv) == 4:
	variable = sys.argv[3]
else:
	variable = sys.argv[2] + '_data'

output.write('char ' + variable + '[] = {')

for char in input[:-1]:
	output.write(hex(ord(char)) + ', ')

output.write(hex(ord(input[-1])) + '};\n')

output.close()
