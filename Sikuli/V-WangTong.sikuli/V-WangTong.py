from os.path import expanduser

TASK_DATA_PATH = "data\\yeyaya-2.xls"
NOX_PATH = expanduser("~") + "\\AppData\\Roaming\\Nox\\bin\\Nox.exe"
DEBUG = 0
APP = "Salary"
#APP = "YuQing"
APP = "KaoQin"

Settings.MoveMouseDelay = 0.12

def clearToMain(toRaise):
    import time
    if toRaise == None: toRaise = True    
    current_milli_time = lambda: int(round(time.time() * 1000))
    time0 = current_milli_time()
    while (not exists("android-main-app-center.png")):
        click("android-back.png")
        time.sleep(0.1)
        click("android-back.png")
        click("android-show-main.png")
        time.sleep(0.4)
        if (current_milli_time() - time0 > 10000):
            if toRaise:
                raise FindFailed('stuck in clearToMain()')
            else:
                break


# if IMEI is empty string, create one
def setNoxMEID(phoneNum, IMEI):
    clearToMain(True)
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

def logonVWT(phoneNum, password):
    if phoneNum != "":
        wait("vwt-logon-account.png")
        r = find("vwt-logon-account.png").right(100)
        doubleClick(r)
        type(Key.BACKSPACE)
        paste(phoneNum)
        time.sleep(0.2)

    wait("vwt-logon-password.png")
    r = find("vwt-logon-password.png").right(80)
    doubleClick(r)
    type(Key.BACKSPACE)
    paste(password)
    time.sleep(0.15)

    click("vwt-log-logon.png")
    time.sleep(0.25)
    # sometimes, the below message may appear too slowly, or stay for too long, need to wait vanish twice
    waitVanish("vwt-v-is-working.png")
    waitVanish("vwt-v-is-working.png")

    if exists("vwt-logon-password-err.png"):
        click("vwt-pass-err-close.png")
        print "logonVWT(" + phoneNum + ", " + password + "), return False"
        return False
    else:
        waitVanish("vwt-log-updating-phonebook.png")
        waitVanish("vwt-log-updating-phonebook.png")
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
    clearToMain(True)
    click("android-main-vwt-icon.png")
    if not exists("vwt-log-account-pass.png"):
        if exists("vwt-qiye-app-back-close.png"):
            click(find("vwt-qiye-app-back-close.png").right(8))
        if exists("vwt-qiye-app-back.png"):
            click("vwt-qiye-app-back.png")
        if exists("vwt-bar-me.png"):
            click("vwt-bar-me.png")
        if exists("vwt-bar-me-focused.png"):
            print "logoutVWT(True)"
            logoutVWT(True)

    ok = logonVWT(phoneNum, "111111")
    if not ok: ok = logonVWT("", "123321")
    if not ok: ok = logonVWT("", "123123")

    return ok;


# return value see vwtSalary
def vwtYuQing(phoneNum, changeMeid):
    if changeMeid:
        setNoxMEID(phoneNum, "")

    ok = startVWT(phoneNum)
    if not ok: return '2-VWT Logon Error'

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
# return '2-V网通登录错误'
# return '3-不是薪酬通用户'
# return '4-未知错误'
def vwtSalary(phoneNum, changeMeid):
    if changeMeid:
        setNoxMEID(phoneNum, "")

    ok = startVWT(phoneNum)
    if not ok: return '2-VWT Logon Error'

    time.sleep(0.7)
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
        time.sleep(0.2)
        if exists("vwt-work-i-see2.png"): click("vwt-work-i-see2.png")

    click("vwt-work-qiye-app.png")
    click("vwt-app-bar-salary.png")

    time.sleep(0.3)
    wait("vwt-app-slary-title.png")
    click("vwt-app-mysalary.png")
    time.sleep(0.7)

    ret = '' # unknown result by defaut
    if exists("vwt-app-slary-auth-title.png"):
        wait("vwt-salayapp-log-passwd.png")
        r = find("vwt-salayapp-log-passwd.png").right(80)
        doubleClick(r)
        type(Key.BACKSPACE)
        paste("123123")
        time.sleep(0.2)

        click("vwt-salaryapp-logon-btn.png")
        time.sleep(0.3)

        if exists("vwt-salaryapp-passwd-error.png"):
            ret = '4-Salary App Logon Error'
        elif exists("vwt-salaryapp-my-salary-title.png"):
            ret = '1'
        elif exists("vwt-slaryapp-slelct-one-corp.png"):
            r = find("vwt-slaryapp-slelct-one-corp.png").below(60)
            click(r)
            time.sleep(0.2)
            if exists("vwt-salaryapp-my-salary-title.png"):
                ret = '1'
    else:
        if exists("vwt-salaryapp-not-user.png"):
            ret = '3-Not Salary User'
            click("vwt-salaryapp-not-user-close.png")

    click("vwt-salaryapp-close.png")
    click("android-back.png")
    time.sleep(0.5)
    click("vwt-bar-me.png")
    logoutVWT(True)
    click("android-show-main.png")
    time.sleep(0.4)
    return ret;



# return '1': success
# return '2-V网通登录错误'
def vwtKaoQin(phoneNum, changeMeid):
    if changeMeid:
        setNoxMEID(phoneNum, "")

    ok = startVWT(phoneNum)
    if not ok: return '2-VWT Logon Error'

    time.sleep(0.7)
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
        time.sleep(0.2)
        if exists("vwt-work-i-see2.png"): click("vwt-work-i-see2.png")

    time.sleep(0.5)
    r = find("vwt-app-work-circle-icon.png")
    r.hover()

    wheel(WHEEL_DOWN, 4)
    click("vwt-kaoqin-app-icon.png")
    wait("vwt-kaoqin-mobile-kaoqin.png")
    click("vwt-kaoqin-loc-checkin.png")
    time.sleep(0.8)

    if exists("vwt-checkin-liketiyan.png"):
        click("vwt-checkin-liketiyan.png")

    click("vwt-kaoqin-checkin.png")
    time.sleep(0.5)
    click("vwt-kaoqin-checkin-confirm.png")
    time.sleep(0.5)

 
    click("vwt-salaryapp-close.png")
    time.sleep(0.5)
    click("vwt-bar-me.png")
    logoutVWT(True)
    click("android-show-main.png")
    time.sleep(0.4)
    return '1';


def doTask(phoneNum, changeMeid):
    if "Salary" == APP:
        return vwtSalary(phoneNum, changeMeid)
    elif "YuQing" == APP:
        return vwtYuQing(phoneNum, changeMeid)
    elif "KaoQin" == APP:
        return vwtKaoQin(phoneNum, changeMeid)

def restartAndroid():
    import subprocess 
    clearToMain(False)
    click("nox-close.png")
    wait("nox-sure-to-close-emu.png")
    click("nox-restart-confirm.png")
    time.sleep(12)

    subprocess.Popen([NOX_PATH, ''])
    time.sleep(20)
    
    clearToMain(False)


def main():
    import xlrd
    import xlutils.copy
    global exceptionCount

    rb = xlrd.open_workbook(os.path.join(getBundlePath(), TASK_DATA_PATH))
    rs = rb.sheet_by_index(0)
    wb = xlutils.copy.copy(rb)
    ws = wb.get_sheet(0)

    # assume it was successful
    lastRet = '1'
    exceptionCount = 0;
    for rownum in range(1, rs.nrows):
        phoneNum = str(rs.row_values(rownum)[0])
        if phoneNum.find('.') > 0:
            phoneNum = phoneNum.split('.')[0]
        if phoneNum == '': continue

        done = str(rs.row_values(rownum)[1])
        if '' == done:
            print 'Task: [',rownum,']:', 'phoneNum =', phoneNum
            ret = ''

            if DEBUG == 1:
                ret = doTask(phoneNum, lastRet == '1')
                lastRet = ret
                exceptionCount = 0
                ws.write(rownum, 1, ret)
                ws.write(rownum, 2, APP)
                wb.save(os.path.join(getBundlePath(), TASK_DATA_PATH))
            else:
                try:
                    ret = doTask(phoneNum, lastRet == '1')
                    lastRet = ret
                    exceptionCount = 0
                    ws.write(rownum, 1, ret)
                    ws.write(rownum, 2, APP)
                    wb.save(os.path.join(getBundlePath(), TASK_DATA_PATH))

                except FindFailed:
                    exceptionCount = exceptionCount + 1
                    lastRet = 'exception'
                    print "exception FindFailed found, continuous count = ", exceptionCount

        if exceptionCount >= 3:
            restartAndroid()
            exceptionCount = 0
    return

main()
main()
main()
main()
