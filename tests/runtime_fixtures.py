"""Small original-format synthetic fixtures; contains no commercial assets."""
import struct
import zlib

def p32(*values):
    return struct.pack('<'+'I'*len(values), *(v & 0xffffffff for v in values))

class Script:
    def __init__(self):
        self.data=bytearray();self.elements=[];self.code=[];self.functions={}
    def string_data(self,text):
        at=len(self.data);self.data.extend(text.encode('cp932')+b'\0');return at
    def number_data(self,value):
        at=len(self.data);self.data.extend(p32(value));return at
    def element(self,*fields):
        index=len(self.elements);self.elements.append(fields);return index
    def value(self,value):
        if isinstance(value,int):return self.element(4,0,self.number_data(value),0)
        return self.element(5,0,self.string_data(value),0)
    def bank(self,name,index):
        return self.element(8,self.string_data(name),0,self.number_data(index))
    def call(self,name,*args):
        if name not in self.functions:self.functions[name]=self.element(0,self.string_data(name),0,0)
        indices=[self.value(v) for v in args];self.code.extend([self.functions[name],len(indices),*indices])
    def assign(self,bank,index,value):
        self.code.append(self.bank(bank,index));self.call('=',value)
    def control(self,name,link=-1):
        slot=self.number_data(link);index=self.element(1,self.string_data(name),slot,0);return index,slot
    def block(self,body):
        opening,link=self.control('{');begin=len(self.code);self.code.append(opening)
        body(self)
        closing,back=self.control('}',begin);end=len(self.code);self.code.append(closing)
        struct.pack_into('<I',self.data,link,end)
    def finish(self):
        code=p32(*self.code);index=b''.join(p32(*e) for e in self.elements);payload=bytes(b^0xaa for b in self.data)
        return b'YKS001\x01\0'+p32(48,0,48,len(self.code),48+len(code),len(self.elements),48+len(code)+len(index),len(payload),0,0)+code+index+payload

def archive(files):
    data=bytearray(24);data[:8]=b'YKC001\0\0';data[8:12]=p32(24);entries=[]
    for name,blob in files:
        encoded=name.encode('cp932')+b'\0';name_at=len(data);data.extend(encoded);offset=len(data);data.extend(blob)
        entries.append(p32(name_at,len(encoded),offset,len(blob),0))
    index_at=len(data);data.extend(b''.join(entries));data[16:24]=p32(index_at,len(entries)*20);return bytes(data)

def png(width=32,height=32,color=(40,60,80,255)):
    def chunk(kind,body):return struct.pack('>I',len(body))+kind+body+struct.pack('>I',zlib.crc32(kind+body))
    raw=(b'\0'+bytes(color)*width)*height
    return b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',width,height,8,6,0,0,0))+chunk(b'IDAT',zlib.compress(raw))+chunk(b'IEND',b'')

def read_png(path):
    data=path.read_bytes();assert data[:8]==b'\x89PNG\r\n\x1a\n';at=8;compressed=bytearray()
    while at<len(data):
        size=struct.unpack_from('>I',data,at)[0];kind=data[at+4:at+8];body=data[at+8:at+8+size];at+=size+12
        if kind==b'IHDR':w,h,depth,color,*_=struct.unpack('>IIBBBBB',body);assert depth==8 and color in (2,6);channels=3 if color==2 else 4
        if kind==b'IDAT':compressed.extend(body)
    raw=zlib.decompress(compressed);stride=w*channels;rows=[];previous=bytearray(stride);at=0
    for _ in range(h):
        mode=raw[at];at+=1;row=bytearray(raw[at:at+stride]);at+=stride
        for i in range(stride):
            a=row[i-channels] if i>=channels else 0;b=previous[i];c=previous[i-channels] if i>=channels else 0
            if mode==1:predict=a
            elif mode==2:predict=b
            elif mode==3:predict=(a+b)//2
            elif mode==4:
                p=a+b-c;dist=[abs(p-a),abs(p-b),abs(p-c)];predict=[a,b,c][dist.index(min(dist))]
            elif mode==0:predict=0
            else:raise ValueError('invalid PNG filter')
            row[i]=(row[i]+predict)&255
        rows.append(row);previous=row
    return w,h,lambda x,y:tuple(rows[y][x*channels:x*channels+3])
