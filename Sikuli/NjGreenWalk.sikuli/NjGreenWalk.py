USER = '1385***4288'
PASSWORD = '******'
IMEI = ''

# if IMEI is an empty string, create one
def setNoxMEID(phoneNum, IMEI):
    clearToMain()
    find("nox-simulator.png")
    click("nox-setup.png")
    wait("systemConfig.png")
    click("property-setup.png")

    r = find("phone-num.png").right(100)
    doubleClick(r)
    type(Key.BACKSPACE)
    paste(phoneNum)

    if "" != IMEI:
        r = find("ImeiConfig.png").right(100)
        doubleClick(r)
        type(Key.BACKSPACE)
        paste(IMEI)
    else:
        click("sys-setting-imei-create.png")

    click("save-config.png")
    if exists("sysUpdateSuccess.png"):
        click("OK.png")
    r = find("systemConfig.png").right(600)
    click(r.find("close.png"))

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

    setNoxMEID(USER, IMEI)
    clearToMain()
    logOn(USER, PASSWORD)
    clearToMain()

    for k in range(20000):
        #print(str(datetime.datetime.now()) + ': k=' + str(k))
        click("nox-right-bar-shake.png")
        time.sleep(0.5)

main()
