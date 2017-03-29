import os,sys
import matplotlib.pyplot as plt

bench_range = {"fullmcf":2000, "fullastar":300, "fullbzip2":400, "fullbwaves":2000, "fullgcc":500, "fullmilc":1200, "fullzeusmp":800, "fullcactus":600, "fullgems":1000, "fulllbm":600, "fullsoplex":400, "fullcalculix":100, "fullhmmer":100, "fullsjeng":300, "fulllib":100, "fullh264":100, "fulltonto":100, "fullomnetpp":200, "fullsphinx3":100, "fakestage":1000}

if len(sys.argv) != 2:
	print "not enough parameter!"
	sys.exit()

benchmark = sys.argv[1]
#sample_rate_list = [x * 8 for x in range(1, 11)]
#sample_rate_list = [x * 64 for x in range(0, 2)]
sample_rate_list = [64, 128, 256, 512]
#sample_rate_list = [1, 64]
print sample_rate_list
dis = 10
start = 10
thresh = 0.01

hot_set_size = 64 
for i in range(0, 20):
	fig = plt.figure()
	plt.title("wss tendency graph")
	thresh = 0.01 * (i + 1)
	base_xsize = 0
	for sample_rate in sample_rate_list: 
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

		enlarge = base_xsize / a_x[-1]
		#print "thresh:{0} {1}".format(a_x[-1], enlarge)
		a_x = [x * enlarge for x in a_x]
		#print a_x[-1]
		plt.plot(a_x, aet_wss, '.-' , label="1/" + str(sample_rate))
	plt.ylim(0, bench_range[benchmark])	
	plt.legend()
	plt.savefig("./pic/" + benchmark + "_" + str(thresh) + "_wss_tendency.png")	
	plt.close()


				

			
