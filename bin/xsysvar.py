#!/usr/bin/env python3

my_header = {
    'Accept': 'text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8',
    'Accept-Language': 'zh-CN,zh;q=0.8,en-US;q=0.5,en;q=0.3',
    'Accept-Encoding': 'gzip, deflate',
    'Referer': 'https://127.0.0.1', 
    'Connection': 'keep-alive',
    'Cache-Control': 'max-age=0',
    'Host':None
}

import requests
#URL you want to access
geturl ="https://127.0.0.1:10443/zosmf/variables/rest/1.0/systems/ADCDPL.S0W1"

#By default, you start to disable SSL verification
res = requests.get(geturl,cert=('/tmp/zosmf-crt.pem', '/tmp/zosmf-key.pem'),headers=my_header,verify=False)

print(res.status_code)
print(res.text)

