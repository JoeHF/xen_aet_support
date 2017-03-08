import os,sys
import matplotlib.pyplot as plt
if len(sys.argv) != 2:
	print "not enough parameter!"
	sys.exit()

benchmark = sys.argv[1]
shell_cmd = "ls ./temp/" + benchmark
output = os.popen(shell_cmd)
output = output.read()

output = output.split()
err_rate_file = open("./temp/" + benchmark + "/" + "tmp.txt", "a")
range_limit = 10 
err_rate_all = 0
dis = 10
start = 10
for i in range(0, range_limit):
	lru_wss = []
	aet_wss = []
	y_max = 0
	thresh = 0.01 * (i + 1)
	l_wss = 0
	a_wss = 0
	l_skip = 0
	a_skip = 0
	l_x = []
	lx = start 
	a_x = []
	ax = start 
	for file in output:
		if "lru_mc" in file and "png" not in file:
			lx += dis	
			fo = open("./temp/" + benchmark + "/" + file)
			wss = 0
			for line in fo.readlines():
				wss += 0.00390625 # 1/256 
				if float(line) < thresh:
					break
				#print "{0}:{1}".format(wss, line)
			if float(line) >= thresh:
				#print file
				l_skip += 1
			else:	
				if wss > y_max:
					y_max = wss
				l_x.append(lx)
				l_wss = wss
				lru_wss.append(l_wss)

	#print "-----"
	valid = 0
	invalid = 0
	for file in output:
		if "miss_curve" in file and "png" not in file:
			ax += dis 
			fo = open("./temp/" + benchmark + "/" + file)
			wss = 0
			odd = 0
			for line in fo.readlines():
				if odd % 2 == 1:
					wss += 1
					if float(line) < thresh:
						break
					#print "{0}:{1}".format(wss, line)
				odd = (odd + 1) % 2	
			if float(line) >= thresh:
				a_skip += 1
				invalid += 1
			else:	
				valid += 1
				if wss > y_max:
					y_max = wss
				a_x.append(ax)
				a_wss = wss
				aet_wss.append(a_wss)

	print "thresh:{2} aet valid/tot:{0}/{1}".format(valid, valid + invalid, thresh)
	#print "lru skip:{0} aet skip:{1}".format(l_skip, a_skip)
	tot_err_rate = 0
	warm_up = 0
	noise = 0
	for j in range(len(aet_wss), len(aet_wss)):
		if lru_wss[j] == 0:
			noise += 1
			continue
		err_rate = abs(aet_wss[j] - lru_wss[j]) / lru_wss[j]
		#if abs(aet_wss[j] - lru_wss[j]) / lru_wss[j] > 1 or abs(aet_wss[j] - lru_wss[j]) / aet_wss[j] > 1:
		if err_rate > 1.0:
			noise += 1
			continue
		tot_err_rate += err_rate
		#tot_err_rate += 1.0 / err_rate

	avg_err_rate = tot_err_rate / (len(aet_wss) - warm_up - noise)
	#avg_err_rate = 0 
	#avg_err_rate = len(aet_wss) / tot_err_rate
	err_rate_all += avg_err_rate
	err_rate_str = "{0} error rate is:{1} noise:{2} total:{3}\n".format(thresh, avg_err_rate, noise, len(aet_wss))
	err_rate_file.write(err_rate_str)
	"""
	aet_wss_adjust = []
	for j in range(len(aet_wss)):
		if j >= 3:
			aet_wss_adjust.append(aet_wss[j - 3] * 0.1 + aet_wss[j - 2] * 0.2 + aet_wss[j - 1] * 0.3 + aet_wss[j] * 0.4)
		else:
			aet_wss_adjust.append(aet_wss[j])
	"""		

	xx = [x*15 for x in range(1 , len(lru_wss) + 1)]

	fig = plt.figure()
	plt.plot(a_x, aet_wss, 'r.-', label="aet_wss")
	plt.title("WSS")
	plt.ylim(0, int(y_max * 1.5))
	plt.xlabel("Time(s)")
	plt.ylabel("Wss(MB)")
	plt.legend()
	plt.savefig("./temp/" + benchmark + "/" + str(thresh) + "_cmp_wss_" + benchmark + ".png")
	plt.close('all')


avg_err_rate = err_rate_all / range_limit
err_rate_str = "average error rate is:{0}\n".format(avg_err_rate)
err_rate_file.write(err_rate_str)
err_rate_file.close()



