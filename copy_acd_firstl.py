import sys

def fun(fin, wav, fout):
	fin = open(fin, 'r')
	lines = fin.readlines()
	fin.close()

	wav = open(wav, 'r')
	linesw = wav.readlines()
	wav.close()

	fout = open(fout, 'w')

	fflist = []
	for line in lines:
		part = line.split()
		fflist.append(part[0])

	for line in linesw:
		part = line.split()
		if part[0] in fflist:
			fout.write(line)

	fout.close()

if __name__ == '__main__':
	fin = sys.argv[1]
	wav = sys.argv[2]
	fout = sys.argv[3]
	fun(fin, wav, fout)