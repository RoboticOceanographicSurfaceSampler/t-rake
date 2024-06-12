import os
import json
import time

configurationpath = '/trake/configuration'

deployFile = configurationpath + '/' + '__runfile__.deploy'

time.sleep(5)
if os.path.exists(deployFile):
    os.remove(deployFile)

time.sleep(5)
runfileContent = '{"configurationName": "defaultconfig"}'
with open(deployFile, 'w') as file:
    file.write(runfileContent)
