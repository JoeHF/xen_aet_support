# -*- coding: UTF-8 -*-
import os,sys
# send email
import smtplib
from email.mime.text import MIMEText
from email.mime.application import MIMEApplication 
from email.mime.multipart import MIMEMultipart
from email.mime.image import MIMEImage
from email.header import Header

shell_cmd = "ls ./pic"
output = os.popen(shell_cmd)
output = output.read()

message = MIMEMultipart()
message['From'] = Header("joe", 'utf-8')
message['To'] =  Header("joe", 'utf-8')
message['Subject'] = Header("变化曲线", 'utf-8')

#邮件正文内容
message.attach(MIMEText("变化曲线", 'plain', 'utf-8'))
sender = 'from@runoob.com'
receivers = ['707980114@qq.com']  # 接收邮件，可设置为你的QQ邮箱或者其他邮箱

output = output.split()
for pic in output:
	if ".png" in pic:
		part = MIMEApplication(open("./pic/" + pic,'rb').read())  
		part.add_header('Content-Disposition', 'attachment', filename=pic)  
		message.attach(part)

try:		
	smtpObj = smtplib.SMTP('localhost')
	smtpObj.sendmail(sender, receivers, message.as_string())
	print "邮件发送成功"
except smtplib.SMTPException:
	print "Error: 无法发送邮件"

