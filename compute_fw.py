def loaddata(fin,n):
	fin = open(fin, 'r')
	lines = fin.readlines()
	fin.close()

	data = []
	for line in lines:
		part = line.split()
		ele = []
		if (part[-1] != '['):
			for x in part[:n]:
				ele.append(float(x))
			data.append(ele)
	return data

def transfer(mat):
	data = []
	num = len(mat)
	xlimt = len(mat[0])
	for i in range(xlimt):
		line = []
		for j in range(num):
			line.append(mat[j][i])
		data.append(line)
	return data


def computeMuVar(x):
	# compute mean value and variance for sequence x
	s = sum(x)
	num = len(x)
	mu = s / num
	dif = 0
	for i in x:
		d = i - mu
		dif = dif + pow(d,2)
	var = dif / num - 1
	return mu , var

def cweight(x1,x2):
	#compute f_weight
	mu1 , var1 = computeMuVar(x1)
	mu2 , var2 = computeMuVar(x2)
	fweight = pow( (mu1 - mu2) , 2) / (var1 * var2)
	return fweight

def unitization(w):
	s = sum(w)
	cw = []
	for ele in w:
		cw.append( ele / s )
	return cw

def fun(matx, maty):
	linesnum = len(matx)
	weight = []
	for i in range(linesnum):
		weight.append( cweight(matx[i] , maty[i]) )
	weight = unitization(weight)
	for i in range(23):
		weight[i] = 23 * weight[i]
		p = int(weight[i] * 1000)
		weight[i] = p / 1000.0
	return weight

if __name__ == '__main__':

	datax = loaddata('genuine.ark',23)
	datax = transfer(datax)
	datay = loaddata('replay.ark',23)
	datay = transfer(datay)
	weight = fun(datax, datay)
	print weight
