#!/usr/bin/python3
import sys
from urllib.request import urlopen#用于获取网页
from bs4 import BeautifulSoup#用于解析网页

if len(sys.argv) != 2 :
    print("Usage: " +  sys.argv[0] + " <url>")
    sys.exit(1)

print("Grab all links from: " + sys.argv[1])

html = urlopen(sys.argv[1])
bsObj = BeautifulSoup(html, 'html.parser')
t1 = bsObj.find_all('a')
for t2 in t1:
    t3 = t2.get('href')
    print(t3)
