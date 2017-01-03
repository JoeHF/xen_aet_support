# -*- coding: UTF-8 -*-
import os,sys
# send email
import smtplib
from email.mime.text import MIMEText
from email.mime.application import MIMEApplication 
from email.mime.multipart import MIMEMultipart
from email.mime.image import MIMEImage
from email.header import Header

if len(sys.argv) != 2:
	print "not enough parameter!"
	sys.exit()

benchmark = sys.argv[1]
shell_cmd = "ls ./temp/" + benchmark
output = os.popen(shell_cmd)
output = output.read()

print "-------------"

message = MIMEMultipart()
message['From'] = Header("joe", 'utf-8')
message['To'] =  Header("joe", 'utf-8')
subject = '{0} aet graph'.format(benchmark)
message['Subject'] = Header(subject, 'utf-8')

#邮件正文内容
content = 'all stage miss_curve graph {0}'.format(benchmark)
message.attach(MIMEText(content, 'plain', 'utf-8'))
sender = 'from@runoob.com'
receivers = ['707980114@qq.com']  # 接收邮件，可设置为你的QQ邮箱或者其他邮箱

output = output.split()
for pic in output:
	if ".png" in pic:
		#创建一个带附件的实例
		print pic
		part = MIMEApplication(open("./temp/" + benchmark + "/" + pic,'rb').read())  
		part.add_header('Content-Disposition', 'attachment', filename=pic)  
		message.attach(part)  

try:
	smtpObj = smtplib.SMTP('localhost')
	smtpObj.sendmail(sender, receivers, message.as_string())
	print "邮件发送成功"
except smtplib.SMTPException:
	print "Error: 无法发送邮件"

