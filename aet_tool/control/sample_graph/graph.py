from tool import *
import matplotlib.pyplot as plt
class SampleGraph:
	def __init__(self):
		self.sample_tool = SampleTool()

	def draw_pf_overhead_pic(self, benchmark_name):
		x_axis = self.sample_tool.read_column(benchmark_name, "pf rate")
		y_axis = self.sample_tool.read_column(benchmark_name, "overhead")
		self.draw("pf_rate_overhead_relation", x_axis, y_axis, "pf_rate", "overhead", benchmark_name)

	def draw(self, title, x_axis, y_axis, x_lable, y_lable, benchmark_name):	
		print x_axis
		print y_axis
		fig = plt.figure()
		plt.title(title)
		plt.plot(x_axis, y_axis, 'r.-')
		plt.xlabel(x_lable)
		plt.ylabel(y_lable)
		plt.savefig("pic/" + benchmark_name + "_" + title + ".png")
		plt.close()

sample_graph = SampleGraph()
sample_graph.draw_pf_overhead_pic("mcf")



