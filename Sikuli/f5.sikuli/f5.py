
# Win Key + D, show desktop
#type("d", Key.WIN)

# start processes
import subprocess
mcshield = subprocess.Popen(['C:\\Users\\I078212\\Desktop\\VPN\\mcshield.exe', ''])
f5 = subprocess.Popen(['C:\\Program Files\\F5 VPN\\f5fpclientW.exe', ''])

while (not exists("1514991097697.png")):
    time.sleep(1)

if not exists("1514993164129.png"):
    if exists("1514990789263.png"):
        click("1514990789263.png")
    
    while (not exists("1514990626252.png")):
        time.sleep(1)
    type("1514990709234.png", '321760')

mcshield.terminate()
