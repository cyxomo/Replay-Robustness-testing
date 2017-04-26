import sys

def fun(fin, trl, fout):
	labeldict = {}
	with open(trl, 'r') as t:
		for line in t:
			part = line.split()
			if part[1] == 'genuine':
				labeldict[part[0]] = 1
			else:
				labeldict[part[0]] = 0

	with open(fout, 'w') as foutf:
		with open(fin, 'r') as finf:
			for line in finf:
				part = line.split()
				out = str(labeldict[part[0]]) + ' '
				for ii in part[2:-1]:
					out = out + ii + ' '
				out = out +'\n'
				foutf.write(out)

if __name__ == '__main__':
	fin = sys.argv[1]
	trl = sys.argv[2]
	fout = sys.argv[3]
	fun(fin, trl, fout)
