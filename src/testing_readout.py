import socket
import binascii
import re
import xml.etree.ElementTree as ET
from texttable import Texttable

# https://pypi.org/project/gurux-dlms/
# pip install gurux-dlms
from gurux_dlms import *

# Create a TCP/IP socket
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server_address = ('192.168.1.72', 1342)
sock.connect(server_address)


readout=b''
data=b''

while True:
    readout = sock.recv(1024)
    #print(len(readout))
    #print(binascii.hexlify(readout))
    data += readout
    if readout == b'':
        break

print("sock close")
sock.close()

print(len(data))
data = binascii.hexlify(data)
data = data.decode('utf-8')
print(data)


xml = ""
# decrypting received data
#decrypt = GXDLMSTranslator(TranslatorOutputType.STANDARD_XML)
decrypt = GXDLMSTranslator(TranslatorOutputType.SIMPLE_XML)
decrypt.comments = True
decrypt.security = enums.Security.ENCRYPTION
decrypt.blockCipherKey =GXByteBuffer.hexToBytes("75FD8711381B3FB18C101AEBD192C3AB")

xml = decrypt.messageToXml(GXByteBuffer.hexToBytes(data))
# remove comments in lines to be able to extract multi lines comment next
xml = re.sub("<!--.*?-->", "", xml)


# (?s) interpret whole string as one line for regex
# https://www.regular-expressions.info/refmodifiers.html
commented_xml = re.findall("(?s)<!--.*?-->", xml)
#print(commented_xml[1])

# get rid of first and last line
decrypted_xml = '\n'.join(commented_xml[1].split('\n')[1:-1])
#print(decrypted_xml)

tree = ET.ElementTree(ET.fromstring(decrypted_xml))
#pprint.pprint(ET.tostring(tree, encoding='unicode'))

#print(decrypted_xml)
#for elem in tree.iter():
#    print(elem)

labels = ['Wirkenergie-Import (+A)', 'Wirkenergieexport (-A)', 'Blindenergieimport (+R) (QI+QII)', 'Blindenergieexport (-R) (QIII+QIV)', 'Momentane Wirkenergie Importleistung (+A)', 'Momentane Wirkenergie Exportleistung (-A)']
t = Texttable()
i=0
for elem in tree.iter('UInt32'):
    #print(elem)
    #print(elem.attrib['Value'])
    #print(labels[i], end='\t\t')
    #print(int(elem.attrib['Value'], 16))
    t.add_row([labels[i], f"{int(elem.attrib['Value'], 16):,}"])
    i=i+1

print(t.draw())

