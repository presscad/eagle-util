DEBUG = 0

def clearToMain():
    while (not exists("android-main-app-center.png")):
        click("android-back.png")
        time.sleep(0.1)
        click("android-show-main.png")
        time.sleep(0.4)

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
        r = find("vwt-logon-account.png").right(100)
        doubleClick(r)
        type(Key.BACKSPACE)
        type(r, phoneNum)
    r = find("vwt-logon-password.png").right(80)
    doubleClick(r)
    type(Key.BACKSPACE)
    type(r, password)
    click("vwt-log-logon.png")
    # sometimes, the below message may appear too slowly, or stay for too long, need to wait vanish twice
    waitVanish("vwt-v-is-working.png")
    waitVanish("vwt-v-is-working.png")

    if exists("vwt-log-pass-error.png"):
        click("vwt-pass-err-close.png")
        print "logonVWT(" + phoneNum + ", " + password + "), return False"
        return False
    else:
        print "logonVWT(" + phoneNum + ", " + password + "), return True"
        return True


def logoutVWT(ready):
    if not ready:
        while (exists("vwt-int-back.png")):
            click("android-back.png")
        meButton1 = "vwt-bar-me.png"
        if exists(meButton1):
            click(meButton1)
    click("vwt-me-setup.png")
    click("vwt-setup-exit.png")
    click("vwt-exit-exit.png")


def startVWT(phoneNum):
    clearToMain()
    click("android-main-vwt-icon.png")
    if not exists("vwt-log-account-pass.png"):
        if exists("vwt-qiye-app-back-close.png"):
            click("vwt-qiye-app-back-close.png")
        if exists("vwt-qiye-app-back.png"):
            click("vwt-qiye-app-back.png")
        if exists("vwt-bar-me.png"):
            click("vwt-bar-me.png")
        if exists("vwt-bar-me-focused.png"):
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


def vwtYuQing(phoneNum):
    setNoxMEID(phoneNum, "")
    ok = startVWT(phoneNum)
    if not ok:
        ok = startVWT(phoneNum)
    if ok:
        click("vwt-bar-work.png")
        time.sleep(0.5)
        if exists("vwt-work-i-see1.png"): 
            click("vwt-work-i-see1.png")
            time.sleep(0.5)                
            if exists("vwt-work-i-see2.png"): click("vwt-work-i-see2.png")

        click("vwt-work-qiye-app.png")
        click("vwt-qiye-yuqing-analysis.png")
    
        time.sleep(0.5)
        if exists("vwt-yuqing-splash-skip.png"):
            r = find("vwt-yuqing-splash-skip.png");
            click(r)
        if not exists("vwt-yuqing-news-yuqing.png"): time.sleep(0.5)
        if not exists("vwt-yuqing-news-yuqing.png"): time.sleep(0.5)
        if not exists("vwt-yuqing-news-yuqing.png"): time.sleep(0.5)
        wait("vwt-yuqing-news-yuqing.png")
        r = find("vwt-yuqing-news-all.png").below(80)
        click(r)
        wait("vwt-yuqing-news-details.png")

        click("vwt-yuqing-news-close.png")
        time.sleep(0.5)
        click("android-back.png")
        time.sleep(0.5)
        click("vwt-bar-me.png")
        logoutVWT(True)
        click("android-show-main.png")
        time.sleep(0.4)
    return ok


def restartAndroid():
    clearToMain()
    click("android-right-restart.png")
    wait("nox-whether-restart.png")
    click("nox-restart-confirm.png")
    time.sleep(20)
    clearToMain()


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
                ok = vwtYuQing(phoneNum)
                exceptionCount = 0
                if ok:
                    ws.write(rownum, 1, '1')
                else:
                    # wrong password
                    ws.write(rownum, 1, '2')
                wb.save(os.path.join(getBundlePath(), 'data\\tasks.xls'))
            else:
                try:
                    ok = vwtYuQing(phoneNum)
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

        if exceptionCount >= 3:
            restartAndroid()
            exceptionCount = 0

main()
