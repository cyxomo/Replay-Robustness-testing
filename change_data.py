def fun(finp, fout):
	fout = open(fout, 'w')
	for num in [1,2,3,4,5,6]:
		fin = finp+'/' + str(num) + '/replay/wav.scp'
		with open(fin, 'r') as f:
			for line in f:
				part = line.split()
				gg = part[4].split('/')[6].split('-')[1]
				kk = part[0].split('_')
				out = kk[0] + '_' + kk[1] + '_' + gg+ '_' + kk[-1] + ' '
				for i in part[1:]:
					out = out + i + ' '
				out = out + '\n'
				fout.write(out)

	fout.close()

if __name__ == '__main__':
	finp = 'replay'
	fout = 'replay/wav.scp'
	fun(finp, fout)