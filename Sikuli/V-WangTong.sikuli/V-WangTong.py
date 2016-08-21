DEBUG = 0
APP = "YuQing" # "Salary"

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

    ok = logonVWT(phoneNum, "518518")
    #if not ok: ok = logonVWT("", "112233")

    return ok;

# return '1': success
# return '2': wrong VWT password
def vwtYuQing(phoneNum):
    setNoxMEID(phoneNum, "")
    ok = startVWT(phoneNum)
    if not ok: return '2'

    wait("vwt-bar-work.png")
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
    time.sleep(0.3)

    return '1'


# return '1': success
# return '2': wrong VWT password
# return '3': not Salary App user
def vwtSalary(phoneNum):
    setNoxMEID(phoneNum, "")
    ok = startVWT(phoneNum)
    if not ok:
        return '2'

    time.sleep(1)
    wait("vwt-bar-work.png")
    click("vwt-bar-work.png")
    for x in range(0, 1):
        if exists("vwt-bar-work.png"):
            click("vwt-bar-work.png")
            time.sleep(0.4)
        else:
            break

    if exists("vwt-work-i-see1.png"): 
        click("vwt-work-i-see1.png")
        time.sleep(0.5)                
        if exists("vwt-work-i-see2.png"): click("vwt-work-i-see2.png")

    click("vwt-work-qiye-app.png")
    click("vwt-app-bar-salary.png")

    time.sleep(0.5)
    wait("vwt-app-slary-title.png")
    click("vwt-app-mysalary.png")
    time.sleep(0.8)

    ret = '1'
    if exists("vwt-salaryapp-not-user.png"):
        ret = '3'
        click("vwt-salaryapp-not-user-close.png")
    else:
        wait("vwt-app-slary-auth-title.png")
    
        r = find("vwt-salayapp-log-passwd.png")
        click(r)
        type(r, "123123")
    
        click("vwt-salaryapp-logon-btn.png")
        time.sleep(0.5)
        wait("vwt-salaryapp-my-salary-title.png")


    click("vwt-salaryapp-close.png")
    click("android-back.png")
    time.sleep(0.5)
    click("vwt-bar-me.png")
    logoutVWT(True)
    click("android-show-main.png")
    time.sleep(0.4)
    return ret;

def doTask(phoneNum):
    if APP == "Salary":
        return vwtSalary(phoneNum)
    else:
        return vwtYuQing(phoneNum)

def restartAndroid():
    clearToMain()
    click("android-right-restart.png")
    wait("nox-whether-restart.png")
    click("nox-restart-confirm.png")
    time.sleep(25)
    clearToMain()


def main():
    import xlrd
    import xlutils.copy

    rb = xlrd.open_workbook(os.path.join(getBundlePath(), 'data\\tasks.xls'))
    rs = rb.sheet_by_index(0)
    wb = xlutils.copy.copy(rb)
    ws = wb.get_sheet(0)

    exceptionCount = 0;
    for rownum in range(1, rs.nrows):
        phoneNum = str(rs.row_values(rownum)[0])
        if phoneNum.find('.') > 0:
            phoneNum = phoneNum.split('.')[0]
        if phoneNum == '': continue

        done = str(rs.row_values(rownum)[1])
        print '[',rownum,']:', 'phoneNum =', phoneNum, ', done=', done

        if done == '':
            if DEBUG == 1:
                ret = doTask(phoneNum)
                exceptionCount = 0
                ws.write(rownum, 1, ret)
                wb.save(os.path.join(getBundlePath(), 'data\\tasks.xls'))
            else:
                try:
                    ret = doTask(phoneNum)
                    exceptionCount = 0
                    ws.write(rownum, 1, ret)
                    wb.save(os.path.join(getBundlePath(), 'data\\tasks.xls'))

                except FindFailed:
                    exceptionCount = exceptionCount + 1
                    print "exception FindFailed found, continuous count = ", exceptionCount

        if exceptionCount >= 3:
            restartAndroid()
            exceptionCount = 0
    return

main()
main()
main()
