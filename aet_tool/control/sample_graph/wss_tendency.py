import os,sys
import matplotlib.pyplot as plt

bench_range = {"fullmcf":2000, "fullastar":300, "fullbzip2":400, "fullbwaves":2000, "fullgcc":500, "fullmilc":1200, "fullzeusmp":800, "fullcactus":600, "fullgems":1000, "fulllbm":600, "fullsoplex":400, "fullcalculix":100, "fullhmmer":100, "fullsjeng":300, "fulllib":100, "fullh264":100, "fulltonto":100, "fullomnetpp":200, "fullsphinx3":100, "fakestage":1000}

if len(sys.argv) != 2:
	print "not enough parameter!"
	sys.exit()

benchmark = sys.argv[1]
#sample_rate_list = [x * 8 for x in range(1, 11)]
sample_rate_list = [x * 16 for x in range(1, 6)]
print sample_rate_list
dis = 10
start = 10
thresh = 0.01

for i in range(0, 20):
	fig = plt.figure()
	plt.title("wss tendency graph")
	thresh = 0.01 * (i + 1)
	for sample_rate in sample_rate_list: 
		cmd = "ls /new2/temp_sample_{0}/{1}".format(sample_rate, benchmark)
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
				fo = open("/new2/temp_sample_" + str(sample_rate) + "/" + benchmark + "/" + file)
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
			
		plt.plot(a_x, aet_wss, '.-' , label="1/" + str(sample_rate))
	plt.ylim(0, bench_range[benchmark])	
	plt.legend()
	plt.savefig("./pic/" + str(thresh) + "_wss_tendency.png")	
	plt.close()


				

			
