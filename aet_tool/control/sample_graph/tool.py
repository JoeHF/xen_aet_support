#!/usr/bin/env python
# -*- coding: utf-8 -*-
# 读取excel数据
# 小罗的需求，取第二行以下的数据，然后取每行前13列的数据
import xlrd

class SampleTool:
	def __init__(self):
		self.data = xlrd.open_workbook('aet_sample_result.xlsx')
		self.benchmark_map = {'mcf': 1, 'milc': 2, 'zeusmp': 3, 'cactus': 4, 'gems': 5, 'lbm': 6, 'soplex': 7, 'sjeng': 8, 'omnetpp': 9}
		self.valid_sheets = range(1, 6)
		self.sample_rate = [8, 16, 32, 64, 128]
		# 获得标题 
		self.column_map = {}
		table = self.data.sheets()[1]
		for i in range(1, 7):
			row_values = table.row_values(0)
			self.column_map[row_values[i]] = i

		print self.column_map	
	
	def read_benchmark(self, benchmark_name):
		result = []
		for i in self.valid_sheets:
			table = self.data.sheets()[i]
			nrows = table.nrows
			for j in range(nrows):
				if j == self.benchmark_map[benchmark_name]:
					result.append(table.row_values(j)[:10])

		return result			

	def read_column(self, benchmark_name, column_name):
		benchmark_result = self.read_benchmark(benchmark_name)
		index = self.column_map[column_name]
		column_result = []
		for i in range(len(benchmark_result)):
			column_result.append(benchmark_result[i][index])

		return column_result	

