
def clearToMain():
    click("android-back.png")
    while (not exists("noxAppCenterIcon.png")):
        click("android-back.png")


def setNoxMEID(phoneNum, IMEI):
    clearToMain()
    find("nox-simulator.png")
    click("nox-setup.png")
    wait("systemConfig.png")
    click("property-setup.png")

    r = find("phone-num.png").right(100)
    doubleClick(r)
    type(Key.BACKSPACE)
    type(r, phoneNum)

    r = find("ImeiConfig.png").right(100)
    doubleClick(r)
    type(Key.BACKSPACE)
    type(r, IMEI)

    click("save-config.png")
    if exists("sysUpdateSuccess.png"):
        click("OK.png")
    r = find("systemConfig.png").right(600)
    click(r.find("close.png"))

def logonVWT(phoneNum, password):
    r = find("vwt-accountLabel.png").right(100)
    doubleClick(r)
    type(Key.BACKSPACE)
    type(r, phoneNum)
    r = find("vwt-passwordLabel.png").right(80)
    doubleClick(r)
    type(Key.BACKSPACE)
    type(r, password)
    click("logonButton.png")
    waitVanish("v_is_working.png")
    if exists("vwt-reenter-passwd.png"):
        click("vwt-close.png")
        return False
    return True


def logoutVWT(ready):
    if not ready:
        while (exists("vwt-back.png")):
            click("android-back.png")
        meButton1 = "vwt-meUnfocused.png"
        if exists(meButton1):
            click(meButton1)
    click("vwt-setup.png")
    click("exit-logon.png")
    click("quit-button.png")


def startVWT(phoneNum):
    clearToMain()
    click("v-wang-tong-icon.png")
    if not exists("account-password.png"):
        if exists("vwt-meUnfocused.png"):
            click("vwt-meUnfocused.png")
        if exists("vwt-meFocused.png"):
            print "logoutVWT(True)"
            logoutVWT(True)

    ok = logonVWT(phoneNum, "123123")
    if not ok:
        ok = logonVWT(phoneNum, "123321")
    if not ok:
        ok = logonVWT(phoneNum, "112233")
    if not ok:
        ok = logonVWT(phoneNum, "321321")
    if not ok:
        ok = logonVWT(phoneNum, "518518")


phoneNum = "13851814288"
IMEI = "864394010761872"
setNoxMEID(phoneNum, IMEI)
startVWT(phoneNum)
logoutVWT(False)
