# -*- coding: UTF-8 -*-
import numpy as np
import sys, getopt

import matplotlib.pyplot as plt
import matplotlib.patches as patches
import matplotlib.path as path
import matplotlib.animation as animation

# send email
import smtplib
from email.mime.text import MIMEText
from email.mime.application import MIMEApplication 
from email.mime.multipart import MIMEMultipart
from email.mime.image import MIMEImage
from email.header import Header

aet_file_x_limit = 10000
miss_curve_x_limit = 2000
aet1 = 0
def read_aet_file(xaxis, yaxis, benchmark):
	global aet1
	fo = open("./data/" + benchmark + "/aet_hist.txt")
	tot = 0
	y_tot = 0
	first = 0
	for line in fo.readlines():
		first += 1
		if first == 2:
			strs = line.split()
			y = strs[1].split(':')[1]
			aet1 = int(y)
			continue

		strs = line.split()
		x = strs[0].split(':')[1]
		if int(x) > aet_file_x_limit:
			break
		y = strs[1].split(':')[1]
		y_tot += int(y)
		tot += int(y)
		if int(x) % 20 == 0:
			xaxis.append(int(x))
			yaxis.append(y_tot)
			y_tot = 0
	print "total aet hist count:{0}".format(tot)
	fo.close()

def read_miss_curve_file(xaxis, yaxis, name):
	fo = open(name)
	odd = 0
	tot = 0
	for line in fo.readlines():
		if odd % 2 == 1:
			tot += 1
			if tot > 10:
				if tot > miss_curve_x_limit:
					break
				xaxis.append(tot)
				if (float(line) > 0.001):
					yaxis.append(0.0)
				else:	
					yaxis.append(float(line))
		odd = (odd + 1) % 2


if __name__=="__main__":
	#opts, args = getopt.getopt(sys.argv[1:], "m:r:")
	opts, args = getopt.getopt(sys.argv[1:], "b:n:")
	for op, value in opts:
		if op == "-n":
			name = value
		elif op == "-b":
			benchmark = value

	x_axis = []
	y_axis = []
	read_miss_curve_file(x_axis, y_axis, name)
	miss_curve_graph = name + "_" + benchmark + ".png"
	plt.figure(2)
	plt.title(name)
	plt.plot(x_axis, y_axis)
	plt.savefig(miss_curve_graph)
