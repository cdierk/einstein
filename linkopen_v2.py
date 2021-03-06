import webbrowser
import serial
import re
import pyperclip as clipb
import keyboard


def isValid(serBuff,start_doc,end_doc):
	
	#starts = serBuff.allindices(start_doc)
	#ends = serBuff.allindices(end_doc)
	starts = [match.start() for match in re.finditer(re.escape(start_doc), serBuff)]
	ends = [match.start() for match in re.finditer(re.escape(end_doc), serBuff)]
	
	#print("starts")
	#print(starts)
	#print("ends")
	#print(ends)


	if(starts != -1) and (ends != -1):
		for i in range(0,len(starts)):
			for j in range(0,len(ends)):
				if(ends[j]>starts[i]):
					return [True, starts[i], ends[j]]
		else:
			return [False,-1,-1]
	else:
		return [False, -1, -1]

def processData(data, start_index, end_index):
	#data = "Hello! 0x00 0x00 0x00 *0x00 0x00 0x00 0x00 *0x00 0x00 0x00 0x00 *0x00 0x00 0x00 0x00 &+^Hello! Found chip PN532 Firmware ver. 1.6 Waiting for an ISO14443A Card ... Found an ISO14443A card UID Length: 7 bytes UID Value: 0x04 0x3E 0x85 0x72 0x08 0x4F 0x80 Seems to be an NTAG2xx tag (7 byte UID) $@#*0x04 0x3E 0x85 0x72 *0x08 0x4F 0x80 0x00 *0x44 0x00 0x00 0x00 *0xE1 0x10 0x6D 0x00 *0x03 0x17 0xD1 0x01 *0x13 0x55 0x01 0x63 *0x68 0x72 0x69 0x73 *0x74 0x69 0x6E 0x65 *0x64 0x69 0x65 0x72 *0x6B 0x2E 0x63 0x6F *0x6D 0xFE 0x00 0x00 *0x00 0x00 0x00 0x00 *0x00 0x00 0x00 0x00 *0x00 0x00 0x00 0x00 *0x00 0x00 0x00 0x00 *0x00 0x00 0x00 0x00 *0x00 0x00 0x00 0x00 *0x00 0x00 0x00 0x00 *0x00 0x00 0x00 0x00 *0x00 0x00 0x00 0x00 *0x00 0x00 0x00 0x00 *0x00 0x00 0x00 0x00 *0x00 0x00 0x00 0x00 *0x00 0x00 0x00 0x00 *0x00 0x00 0x00 0x00 *0x00 0x00 0x00 0x00 *0x00 0x00 0x00 0x00 *0x00 0x00 0x00 0x00 *0x00 0x00 0x00 0x00 *0x00 0x00 0x00 0x00 *0x00 0x00 0x00 0x00 *0x00 0x00 0x00 0x00 *0x00 0x00 0x00 0x00 *0x00 0x00 0x00 0x00 *0x00 0x00 0x00 0x00 *0x00 0x00 0x00 0x00 *0x00 0x00 0x00 0x00 *0x00 0x00 0x00 0x00 *0x00 0x00 0x00 0x00 *0x00 0x00 0x00 0x00 *0x00 0x00 0x00 0x00 *0x00 0x00 0x00 0x00 &+^"
	#print "start:", start_index, "end:", end_index
	#print serBuff
	raw_data = data
	
	
	data = data[start_index+len(start_doc):end_index]
	data = data.translate(None, page_start)
	data = data.translate(None, '\t\t')
	data = data.replace("\n",' ')
	data = data.replace("\r",' ')
	#data = data.replace("0x",' ')
	#data = bytearray.fromhex(data)

	data = data.replace(' ','')
	data = [data[i:i+4] for i in range(0, len(data), 4)]
	data = [int(data[i], 16) for i in range(0, len(data))]

	ndef_start_index = data.index(ndef_start)
	ndef_payload_length = data[ndef_start_index+1]


	message = data[ndef_start_index+2:ndef_start_index+2+ndef_payload_length] #has 254 (FE at the end)
	knownRecord = False
	typeLength = 0

	decMsg =""

	# Analyse the first record

	typeLength = message[1]
	typeCode = message[3:3+typeLength]
	beg = 3+typeLength
	end = len(message) 
	#Check if the record is a known type
	if (message[0]&(0b011) == 0x01):	
		print("Record at position "+str(ndef_start_index)+" is a well known type")
		knownRecord = True

	if (knownRecord):
		if(typeCode == url_code):
			if(message[beg] == 0x01):
				decMsg = decMsg + "http://www."
			elif(message[beg] == 0x02):
				decMsg = decMsg + "https://www."
			elif(message[beg] == 0x03):
				decMsg = decMsg + "http://"
			elif(message[beg] == 0x04):
				decMsg = decMsg + "https://"
			elif(message[beg] == 0x05):
				decMsg = decMsg + "tel:"
			elif(message[beg] == 0x06):
				decMsg = decMsg + "mailto:"
			#decMsg = decMsg+ [ chr(i) for i in message[beg+1:end]] #beg +1 since beg contatins link prefix
			decMsg = decMsg+ ''.join(chr(x) for x in message[beg+1:end])
			return([typeCode,decMsg])
		if(typeCode == text_code):
			decMsg = decMsg+ ''.join(chr(x) for x in message[beg+3:end])
			return([typeCode,decMsg])

ser = serial.Serial('/dev/ttyACM0', 115200)
serBuff =""

start_doc = "$@#"
end_doc = "&+^"
page_start ='*'
check_period = 1000
ndef_start = 0x03
url_code = [0x55]
text_code = [0x54]
content = ''
#data = "$@#*04 3E 85 72  .>?r\n		*08 4F 80 00  .O?.\n		*44 00 00 00  D...\n		*E1 10 6D 00  ?.m.\n		*03 5F(95) 91 02  ._?.\n		*35(53) 53 70 91  5Sp?\n		*01 14 54 02  ..T.\n		*65 6E 4E 54  enNT\n		*41 47 20 49  AG I\n		*32 43 20 45  2C E\n		*58 50 4C 4F  XPLO\n		*52 45 52 51  RERQ\n		*01 19 55 01  ..U.\n		*6E 78 70 2E  nxp.\n		*63 6F 6D 2F  com/\n		*64 65 6D 6F  demo\n		*62 6F 61 72  boar\n		*64 2F 4F 4D  d/OM\n		*35 35 36 39  5569\n		*54 0F 13 61  T..a\n		*6E 64 72 6F  ndro\n		*69 64 2E 63  id.c\n		*6F 6D 3A 70  om:p\n		*6B 67 63 6F  kgco\n		*6D 2E 6E 78  m.nx\n		*70 2E 6E 74  p.nt\n		*61 67 69 32  agi2\n		*63 64 65 6D  cdem\n		*6F FE 00 00  o?..\n		*00 00 00 00  ....\n		*00 00 00 00  ....\n		*00 00 00 00  ....\n		*00 00 00 00  ....\n		*00 00 00 00  ....\n		*00 00 00 00  ....\n		*00 00 00 00  ....\n		*00 00 00 00  ....\n		*00 00 00 00  ....\n		*00 00 00 00  ....\n		*00 00 00 00  ....\n		*00 00 00 00  ....\n		*00 00 00 00  ....\n		&+^"d
#data = "$@#*0x04 0x3E 0x85 0x72\n*0x08 0x4F 0x80 0x00\n*0x44 0x00 0x00 0x00\n*0xE1 0x10 0x6D 0x00\n*0x03 0x17 0xD1 0x01\n*0x13 0x55 0x01 0x63\n*0x68 0x72 0x69 0x73\n*0x74 0x69 0x6E 0x65\n*0x64 0x69 0x65 0x72\n*0x6B 0x2E 0x63 0x6F\n*0x6D 0xFE 0x00 0x00\n*0x67 0x2F 0xFE 0x00\n*0x00 0x00 0x00 0x00\n*0x00 0x00 0x00 0x00\n*0x00 0x00 0x00 0x00\n*0x00 0x00 0x00 0x00\n*0x00 0x00 0x00 0x00\n*0x00 0x00 0x00 0x00\n*0x00 0x00 0x00 0x00\n*0x00 0x00 0x00 0x00\n*0x00 0x00 0x00 0x00\n*0x00 0x00 0x00 0x00\n*0x00 0x00 0x00 0x00\n*0x00 0x00 0x00 0x00\n*0x00 0x00 0x00 0x00\n*0x00 0x00 0x00 0x00\n*0x00 0x00 0x00 0x00\n*0x00 0x00 0x00 0x00\n*0x00 0x00 0x00 0x00\n*0x00 0x00 0x00 0x00\n*0x00 0x00 0x00 0x00\n*0x00 0x00 0x00 0x00\n*0x00 0x00 0x00 0x00\n*0x00 0x00 0x00 0x00\n*0x00 0x00 0x00 0x00\n*0x00 0x00 0x00 0x00\n*0x00 0x00 0x00 0x00\n*0x00 0x00 0x00 0x00\n*0x00 0x00 0x00 0x00\n*0x00 0x00 0x00 0x00\n*0x00 0x00 0x00 0x00\n*0x00 0x00 0x00 0x00\n&+^"

#decMsg = decMsg+str(map(hex,message[beg+1:end])) #neglect 0xFE at end+1
it = 1

while(1):
	while ser.in_waiting:  # Or: while ser.inWaiting():
	    serBuff = serBuff + ser.readline()
    	
	if((len(serBuff)>it*check_period)):
		print("checking now")
		#print(serBuff)
		it = it+1
		[flag, startP, endP] = isValid(serBuff,start_doc,end_doc)
		if(flag):
			content = processData(serBuff, startP, endP)
			typeCode = content[0]
			print "Content is : ",content[1]
			if(typeCode == url_code):
				webpg = content[1]
				webbrowser.open(webpg)  # Go to the link   
				serBuff = ""
			elif(typeCode == text_code):
				content = content[1]
				clipb.copy(content)
				keyboard.press_and_release('ctrl+v')
				serBuff =""
			#break

			
			




