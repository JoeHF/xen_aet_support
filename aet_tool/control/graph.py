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

def read_miss_curve_file(xaxis, yaxis, benchmark):
	fo = open("./data/" + benchmark + "/miss_curve.txt")
	odd = 0
	tot = 0
	for line in fo.readlines():
		if odd % 2 == 1:
			tot += 1
			if tot > miss_curve_x_limit:
				break
			xaxis.append(tot)
			yaxis.append(float(line))
		odd = (odd + 1) % 2


if __name__=="__main__":
	#opts, args = getopt.getopt(sys.argv[1:], "m:r:")
	opts, args = getopt.getopt(sys.argv[1:], "b:a:m:")
	for op, value in opts:
		if op == "-a":
			aet_file_x_limit = int(value)
		elif op == "-m":
			miss_curve_x_limit = int(value)
		elif op == "-b":
			benchmark = value

	aet_hist_graph_name = 'aet_hist_{0}.png'.format(benchmark)
	miss_curve_graph_name = 'miss_curve_{0}.png'.format(benchmark)
	aet_hist_graph_path = 'graph/' + benchmark + '/' + aet_hist_graph_name
	miss_curve_graph_path = 'graph/' + benchmark + '/' + miss_curve_graph_name
	x_axis = []
	y_axis = []
	read_aet_file(x_axis, y_axis, benchmark)
	plt.figure(1)
	plt.title(aet_hist_graph_name)
	plt.plot(x_axis, y_axis)
	plt.savefig(aet_hist_graph_path)

	x_axis = []
	y_axis = []
	read_miss_curve_file(x_axis, y_axis, benchmark)
	plt.figure(2)
	plt.title(miss_curve_graph_name)
	plt.plot(x_axis, y_axis)
	plt.savefig(miss_curve_graph_path)

	sender = 'from@runoob.com'
	receivers = ['707980114@qq.com']  # 接收邮件，可设置为你的QQ邮箱或者其他邮箱

	#创建一个带附件的实例
	message = MIMEMultipart()
	message['From'] = Header("joe", 'utf-8')
	message['To'] =  Header("joe", 'utf-8')
	subject = '{0} aet graph'.format(benchmark)
	message['Subject'] = Header(subject, 'utf-8')

	#邮件正文内容
	content = 'aet重用时间分布 aet hist 1:{0}'.format(aet1)
	message.attach(MIMEText(content, 'plain', 'utf-8'))

	# 构造附件1，传送当前目录下的 test.txt 文件
	#att1 = MIMEText(open('aet_hist.png', 'rb').read(), 'base64', 'utf-8')
	#att1["Content-Type"] = 'application/octet-stream'
	# 这里的filename可以任意写，写什么名字，邮件中显示什么名字
	#att1["Content-Disposition"] = 'attachment; filename="aet_hist.png"'
	#message.attach(att1)

	#fp = open('aet_hist.png','rb')
	#msgImage = MIMEImage(fp.read())
	#fp.close()
	#msgImage.add_header('Content-ID','<meinv_image>')
	#message.attach(msgImage)

	part = MIMEApplication(open(aet_hist_graph_path,'rb').read())  
	part.add_header('Content-Disposition', 'attachment', filename=aet_hist_graph_name)  
	message.attach(part)  

	part = MIMEApplication(open(miss_curve_graph_path,'rb').read())  
	part.add_header('Content-Disposition', 'attachment', filename=miss_curve_graph_name)  
	message.attach(part)  
	try:
		smtpObj = smtplib.SMTP('localhost')
		smtpObj.sendmail(sender, receivers, message.as_string())
		print "邮件发送成功"
	except smtplib.SMTPException:
		print "Error: 无法发送邮件"
	

