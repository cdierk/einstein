start_doc = "$@#"
end_doc = "&+^"
import serial
#ser = serial.Serial('/dev/ttyUSB0', 115200)  # open serial port
#print(ser.name)         # check which port was really used

'''
with serial.Serial('/dev/ttyACM0', 115200) as ser:
     while(1):
     	line = ser.readline()
     	print(line)
'''
def isValid(serBuff,start_doc,end_doc):
	start_index = serBuff.rfind(start_doc)
	end_index = serBuff.rfind(end_doc)

	if(start_index != -1) and (end_index != -1):
		if(end_index > start_index):
			return True
		else:
			return False
	else:
		return False

ser = serial.Serial('/dev/ttyACM0', 115200)
serBuff =""

while(1):
	while ser.in_waiting:  # Or: while ser.inWaiting():
	    serBuff = serBuff + ser.readline()
	    if(isValid(serBuff,start_doc,end_doc)):
	    	print(serBuff)
	    	serBuff = ""




