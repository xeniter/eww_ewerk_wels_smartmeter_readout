import socket
import time
import binascii
import crcmod   #pip3 install crcmod

# https://pypi.org/project/gurux-dlms/
# pip install gurux-dlms
from gurux_dlms import *

# data readout from serial interface, which you get every second
data = "7ea070cf0002002313e0c7e6e700db08534d53677002dfe956200002825507e4a90965d819af74e8e8325aca4d00b0df6b9d3b7f847f95dee24ca51c883641b7be98bc52c07a2d7bee229244a6ccea7cf7130ccdba5c35eae2caa1ea379879eb672fd78b494d6136bc9ad7fba61ff29b3b7e"


xml = ""

# decrypting received data
decrypt = GXDLMSTranslator(TranslatorOutputType.STANDARD_XML)
decrypt.comments = True
decrypt.security = enums.Security.ENCRYPTION
decrypt.blockCipherKey =GXByteBuffer.hexToBytes("75FD8711381B3FB18C101AEBD192C3AB")

xml = decrypt.messageToXml(GXByteBuffer.hexToBytes(data))
print(xml)

