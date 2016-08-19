DEBUG = 0

def clearToMain():
    click("android-back.png")
    time.sleep(0.5)
    click("android-back.png")
    time.sleep(0.5)
    click("android-back.png")
    while (not exists("noxAppCenterIcon.png")):
        click("android-back.png")
        time.sleep(0.5)
        click("android-back.png")

# if IMEI is empty string, create one
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

    if "" != IMEI:
        r = find("ImeiConfig.png").right(100)
        doubleClick(r)
        type(Key.BACKSPACE)
        type(r, IMEI)
    else:
        click("sys-setting-imei-create.png")

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

    ok = logonVWT(phoneNum, "123123")
    if not ok:
        ok = logonVWT("", "123321")
    if not ok:
        ok = logonVWT("", "518518")
    if not ok:
        ok = logonVWT("", "321321")
    if not ok:
        ok = logonVWT("", "112233")
    return ok;


def YuQing(phoneNum):
    setNoxMEID(phoneNum, "")
    ok = startVWT(phoneNum)
    if not ok:
        ok = startVWT(phoneNum)
    if ok:
        click("vwt-lower-work.png")
        time.sleep(0.5)
        if exists("vwt-work-i-know-1.png"): 
            click("vwt-work-i-know-1.png")
            time.sleep(0.5)                
            if exists("vwt-market-i-know-2.png"): click("vwt-market-i-know-2.png")

        click("vwt-work-qiyeyingyong.png")
        click("vwt-qiye-yuqingfenxi.png")
    
        if not exists("vwt-news-yuqing.png"): time.sleep(1)
        if not exists("vwt-news-yuqing.png"): time.sleep(1)
        wait("vwt-news-yuqing.png")
        r = find("vwt-yuqing-all.png").below(80)
        click(r)
        wait("vwt-yuqing-details.png")

        click("android-back.png")
        time.sleep(0.5)
        click("android-back.png")
        time.sleep(0.5)
        click("android-back.png")
        time.sleep(0.5)
        click("vwt-meUnfocused.png")
        logoutVWT(True)
    return ok

def main():
    import xlrd
    from xlutils.copy import copy

    rb = xlrd.open_workbook(os.path.join(getBundlePath(), 'data\\tasks.xls'))
    rs = rb.sheet_by_index(0)
    wb = copy(rb)
    ws = wb.get_sheet(0)

    exceptionCount = 0;
    for rownum in range(1, rs.nrows):
        phoneNum = str(rs.row_values(rownum)[0])
        if phoneNum.find('.') > 0:
            phoneNum = phoneNum.split('.')[0]
        if phoneNum == '': continue
    
        done = str(rs.row_values(rownum)[1])
        print '[',rownum,']:', 'phoneNum =', phoneNum, ', done=', done

        if done != '1' and done != '2':
            if DEBUG == 1:
                ok = YuQing(phoneNum)
                exceptionCount = 0
                if ok:
                    ws.write(rownum, 1, '1')
                else:
                    # wrong password
                    ws.write(rownum, 1, '2')
                wb.save(os.path.join(getBundlePath(), 'data\\tasks.xls'))
            else:
                try:
                    ok = YuQing(phoneNum)
                    exceptionCount = 0
                    if ok:
                        ws.write(rownum, 1, '1')
                    else:
                        # wrong password
                        ws.write(rownum, 1, '2')
                    wb.save(os.path.join(getBundlePath(), 'data\\tasks.xls'))

                except FindFailed:
                    exceptionCount = exceptionCount + 1
                    print "exception FindFailed found, continuous count = ", exceptionCount
                
        if exceptionCount > 3: break

main()
