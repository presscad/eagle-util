
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
    if phoneNum != "":
        r = find("vwt-accountLabel.png").right(100)
        doubleClick(r)
        type(Key.BACKSPACE)
        type(r, phoneNum)
    r = find("vwt-passwordLabel.png").right(80)
    doubleClick(r)
    type(Key.BACKSPACE)
    type(r, password)
    click("logonButton.png")
    # sometimes, the below message may appear too slowly, or stay for too long, need to wait vanish twice
    waitVanish("v_is_working.png")
    waitVanish("v_is_working.png")

    if exists("vwt-password-error.png"):
        click("vwt-close.png")
        print "logonVWT(" + phoneNum + ", " + password + "), return False"
        return False
    else:
        print "logonVWT(" + phoneNum + ", " + password + "), return True"
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

    ok = logonVWT("", "518518")
    if not ok:
        ok = logonVWT(phoneNum, "123123")
    if not ok:
        ok = logonVWT("", "123321")
    if not ok:
        ok = logonVWT("", "112233")
    if not ok:
        ok = logonVWT("", "321321")
    return ok;

def main():
    import xlrd
    from xlutils.copy import copy
    
    rb = xlrd.open_workbook(os.path.join(getBundlePath(), 'data\\tasks.xls'))
    rs = rb.sheet_by_index(0)
    wb = copy(rb)
    ws = wb.get_sheet(0)
    
    for rownum in range(1, rs.nrows):
        phoneNum = str(rs.row_values(rownum)[0])
        if phoneNum.find('.') > 0:
            phoneNum = phoneNum.split('.')[0]
        if phoneNum == '': continue
    
        IMEI = str(rs.row_values(rownum)[1])
        done = str(rs.row_values(rownum)[2])
        print '[',rownum,']:', 'phoneNum =', phoneNum, ', IMEI =', IMEI, ', done=', done
    
        if done != '1':
            setNoxMEID(phoneNum, IMEI)
            ok = startVWT(phoneNum)
            if not ok:
                ok = startVWT(phoneNum)
            logoutVWT(False)
            if ok:
                ws.write(rownum, 2, '1')

        wb.save(os.path.join(getBundlePath(), 'data\\tasks.xls'))

main()
