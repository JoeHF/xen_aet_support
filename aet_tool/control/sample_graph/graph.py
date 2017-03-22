from tool import *
import matplotlib.pyplot as plt
import math
class SampleGraph:
	def __init__(self):
		self.sample_tool = SampleTool()
		#self.benchmark = ["mcf", "milc", "zeusmp", "cactus", "gems", "lbm", "soplex", "sjeng", "omnetpp"]
		self.benchmark = self.sample_tool.read_benchmark_name()
		print self.benchmark
	
	def draw_overhead_tendency(self):
		fig = plt.figure()
		plt.title("overhead tendency")
		for i in range(len(self.benchmark)):
			x_axis = self.sample_tool.read_sample_rate()
			y_axis = self.sample_tool.read_column(self.benchmark[i], "overhead")
			y_axis = [y * 100 - 100 for y in y_axis]
			plt.plot(x_axis, y_axis, '.-', label = self.benchmark[i])
			plt.xlabel("sample rate")
			plt.ylabel("overhead(%)")
		plt.legend()	
		plt.savefig("pic/" + "overhead_tendency" + ".png")
		plt.close()

	def draw_all_pf_overhead_pic(self):
		fig = plt.figure()
		plt.title("pf_rate overhead relationship")
		for i in range(len(self.benchmark)):
			x_axis = self.sample_tool.read_column(self.benchmark[i], "pf rate")
			# log the page fault rate
			x_axis = [math.log(x) for x in x_axis]
			y_axis = self.sample_tool.read_column(self.benchmark[i], "overhead")
			y_axis = [y * 100 for y in y_axis]
			plt.plot(x_axis, y_axis, '.-', label = self.benchmark[i])
			plt.xlabel("pf rate(log)")
			plt.ylabel("overhead(%)")
		plt.xlim(7, 15)
		plt.legend()	
		plt.savefig("pic/" + "pf_rate_overhead_all" + ".png")
		plt.close()

	def draw_pf_overhead_pic(self, benchmark_name):
		fig = plt.figure()
		x_axis = self.sample_tool.read_column(benchmark_name, "pf rate")
		# log the page fault rate
		print x_axis
		x_axis = [math.log(x) for x in x_axis]
		y_axis = self.sample_tool.read_column(benchmark_name, "overhead")
		y_axis = [y * 100 for y in y_axis]
		print y_axis
		plt.plot(x_axis, y_axis, '.-', label = benchmark_name)
		plt.xlabel("pf rate(log)")
		plt.ylabel("overhead(%)")
		plt.title("pf_rate overhead relationship")
		plt.xlim(7, 15)
		plt.legend()
		plt.savefig("pic/" + benchmark_name + "_pf_overhead.png")
		plt.close()

	

sample_graph = SampleGraph()
#sample_graph.draw_pf_overhead_pic("cactus")
#sample_graph.draw_all_pf_overhead_pic()
sample_graph.draw_overhead_tendency()



