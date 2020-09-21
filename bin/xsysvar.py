#!/usr/bin/env python3

header = {
    'X-CSRF-ZOSMF-HEADER': 'zosmf',
}

import requests
url ="https://127.0.0.1:10443/zosmf/variables/rest/1.0/systems/ADCDPL.S0W1"
url ="https://torzdt1.canlab.ibm.com:8136/zosmf/variables/rest/1.0/systems/ADCDPL.S0W1"

res = requests.get(url,cert=('/u/ibmuser/certs/zosmf-crt.pem', '/u/ibmuser/certs/zosmf-key.pem'),headers=header,verify=False)

print(res.status_code)
print(res.text)
