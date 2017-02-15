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
miss_curve_x_limit = 1000
aet1 = 0

def domain_index_to_value(index):
	domain = 256
	value = 0
	step = 1
	while index > domain:
		value += step * domain;
		step *= 2
		index -= domain
	
	while index > 0:
		value += step
		index -= 1

	return value	

def read_aet_file(xaxis, yaxis, name):
	global aet1
	fo = open(name)
	tot = 0
	y_tot = 0
	x = 0
	for line in fo.readlines():
		x += 1
		if x > aet_file_x_limit:
			break
		xx = domain_index_to_value(x)
		if xx > 512000:
			break
		y = int(line) 
		y_tot += int(y)
		tot += int(y)
		#if int(x) % 20 == 0:
		#if y_tot != 0:
			#print xx
		if y_tot != 0:	
			xaxis.append(int(xx))
			yaxis.append(y_tot)
		y_tot = 0
	#print "total aet hist count:{0}".format(tot)
	fo.close()
if __name__=="__main__":
	opts, args = getopt.getopt(sys.argv[1:], "b:n:")
	for op, value in opts:
		if op == "-n":
			name = value
		elif op == "-b":
			benchmark = value

	aet_hist_graph = name + "_" + benchmark + '.png'
	x_axis = []
	y_axis = []
	read_aet_file(x_axis, y_axis, name)
	plt.figure(1)
	plt.title(name)
	plt.plot(x_axis, y_axis)
	plt.savefig(aet_hist_graph)

