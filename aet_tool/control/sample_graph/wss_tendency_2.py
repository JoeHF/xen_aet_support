import os,sys
import matplotlib.pyplot as plt

bench_range = {"fullmcf":2000, "fullastar":300, "fullbzip2":400, "fullbwaves":2000, "fullgcc":500, "fullmilc":600, "fullzeusmp":800, "fullcactus":600, "fullgems":1000, "fulllbm":600, "fullsoplex":400, "fullcalculix":100, "fullhmmer":100, "fullsjeng":300, "fulllib":100, "fullh264":100, "fulltonto":100, "fullomnetpp":200, "fullsphinx3":100, "fakestage":1000}

if len(sys.argv) != 2:
	print "not enough parameter!"
	sys.exit()

benchmark = sys.argv[1]
#sample_rate_list = [x * 8 for x in range(1, 11)]
#sample_rate_list = [x * 64 for x in range(0, 2)]
sample_rate_list = [64, 128, 256, 512]
hot_set_size_list = [64, 128 ,256]
#sample_rate_list = [1, 64]
print sample_rate_list
dis = 10
start = 10
thresh = 0.01
						   
for i in range(0, 20):
	fig, axes = plt.subplots(nrows=3, ncols=4,
						 sharex=True, sharey=True,
						 figsize=(12,12))
	
	thresh = 0.01 * (i + 1)
	base_xsize = 0
	j = -1
	plt.ylim(0, bench_range[benchmark])
	for row in axes:
		j += 1	
		k = -1 
		hot_set_size = hot_set_size_list[j] 
		for col in row:
			k += 1
			sample_rate = sample_rate_list[k]
			if sample_rate == 0:
				sample_rate = 1;
			cmd = "ls /new2/temp_hot_set_fixed_{2}/temp_sample_{0}/{1}".format(sample_rate, benchmark, hot_set_size)
			output = os.popen(cmd)
			output = output.read()

			output = output.split()
			valid = 0
			invalid = 0
			y_max = 0
			a_x = []
			aet_wss = []
			a_skip = 0		
			ax = start
			for file in output:
				if "miss_curve" in file and "png" not in file:
					ax += dis
					fo = open("/new2/temp_hot_set_fixed_" + str(hot_set_size) + "/temp_sample_" + str(sample_rate) + "/" + benchmark + "/" + file)
					wss = 0
					odd = 0
					for line in fo.readlines():
						if odd % 2 == 1:
							wss += 1
							if float(line) < thresh:
								break

						odd = (odd + 1) % 2

					if line.strip(' \n' ) == "-nan" or float(line) >= thresh:
						a_skip += 1
						invalid += 1
					else:
						valid += 1
						if wss > y_max:
							y_max = wss
						a_x.append(ax)
						a_wss = wss
						aet_wss.append(a_wss) 
				
			if base_xsize == 0:
				base_xsize = a_x[-1]
				#print base_xsize

			enlarge = 1
			if len(a_x) > 0:
				enlarge = base_xsize / a_x[-1]
			#print "thresh:{0} {1}".format(a_x[-1], enlarge)
			#a_x = [x * enlarge for x in a_x]
			#print a_x[-1]
			#print "{0} {1} {2}".format(len(sample_rate_list) / 2, (k + 1) / 2, (k + 1) % 2 + 1)
			#plt.subplot(len(sample_rate_list) / 2, (k + 1) / 2, k % 2 + 1)
			ax = [x / 10 for x in a_x]
			col.plot(a_x, aet_wss, 'r.-' , label="1/" + str(sample_rate))
	for ax, col in zip(axes[0,:], ['1/64', '1/128', '1/256', '1/512']):
		ax.set_title(col, size=15)
	 
	for ax, row in zip(axes[:,0], ['hss_64', 'hss_128', 'hss_256']):
		ax.set_ylabel(row, size=15, rotation=0, labelpad=30)			
	plt.savefig("./pic/" + benchmark + "_" + str(thresh) + "_wss_tendency.png")	
	plt.close()


				

			