USER = '1385***4288'
PASSWORD = '******'

def logOn(user, password):
    click("mynanjing-app-icon.png")

def clearToMain():
    import time
    current_time = lambda: int(round(time.time()))
    time0 = current_time()
    while (not exists("android-main-app-center.png")):
        click("android-show-main.png")
        time.sleep(0.4)
        if (current_time() - time0 > 7):
            raise FindFailed('stuck in clearToMain()')

def main():
    import datetime
    
    clearToMain()
    logOn(USER, PASSWORD)
    for k in range(20000):
        #st = datetime.datetime.fromtimestamp(time.time()).strftime('%Y-%m-%d %H:%M:%S')
        #print(st + ': k=' + str(k))
        click("nox-right-bar-shake.png")
        time.sleep(0.8)

main()
