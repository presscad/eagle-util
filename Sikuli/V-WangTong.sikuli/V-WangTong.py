from os.path import expanduser

TASK_DATA_PATH = "data\\"
TASK_DATA_PATH = "C:\Users\Eagle\Dropbox\Temp\VWT\data"

NOX_PATH = expanduser("~") + "\\AppData\\Roaming\\Nox\\bin\\Nox.exe"
DEBUG = 0
#APP = "Salary"
#APP = "YuQing"
APP = "KaoQin"
PASSWORDS = []
NOX_PROCESS = None

Settings.MoveMouseDelay = 0.11

def clearToMain():
    import time
    current_time = lambda: int(round(time.time()))
    time0 = current_time()
    while (not exists("android-main-app-center.png")):
        click("android-back.png")
        time.sleep(0.1)
        click("android-back.png")
        click("android-show-main.png")
        time.sleep(0.4)
        if (current_time() - time0 > 7):
            raise FindFailed('stuck in clearToMain()')


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
    time.sleep(0.2)
    # print("password: " + password)

    click("vwt-log-logon.png")
    time.sleep(1)
    # sometimes, the below message may appear too slowly, or stay for too long, need to wait vanish twice
    waitVanish("vwt-v-is-working.png")
    waitVanish("vwt-v-is-working.png")
    time.sleep(0.5)

    if exists("vwt-logon-password-err.png"):
        click("vwt-pass-err-close.png")
        print "logonVWT(" + phoneNum + ", " + password + "), return False"
        return False
    else:
        if exists("vwt-after-logon-popup.png", 0):
            click("vwt-after-logon-popup-close.png")
        else:
            waitVanish("vwt-log-updating-phonebook.png")
        waitVanish("vwt-log-updating-phonebook.png")
        print "logonVWT(" + phoneNum + ", " + password + "), return True"
        return True


def logoutVWT(ready):
    if not ready:
        if exists("vwt-after-logon-popup.png"):
            click("vwt-after-logon-popup-close.png")
        while (exists("vwt-int-back.png")):
            click("android-back.png")
        meButton1 = "vwt-bar-me.png"
        if exists(meButton1):
            click(meButton1)
    click("vwt-me-setup.png")
    click("vwt-setup-exit.png")
    click("vwt-exit-exit.png")


def startVWT(phoneNum, password):
    clearToMain()
    click("android-main-vwt-icon.png")
    if not exists("vwt-log-account-pass.png"):
        if exists("vwt-after-logon-popup.png", 0):
            click("vwt-after-logon-popup-close.png")
        if exists("vwt-qiye-app-back-close.png"):
            click(find("vwt-qiye-app-back-close.png").right(8))
        if exists("vwt-qiye-app-back.png"):
            click("vwt-qiye-app-back.png")
        if exists("vwt-bar-me.png"):
            click("vwt-bar-me.png")
        if exists("vwt-bar-me-focused.png"):
            print "logoutVWT(True)"
            logoutVWT(True)

    if None == password:
        password = PASSWORDS
    elif len(password) == 0:
        password = PASSWORDS

    if len(password) == 0:
        print "No passord!"

    ok = False
    for i in range(len(password)):
        if 0 == i:
            ok = logonVWT(phoneNum, password[i])
        else:
            ok = logonVWT("", password[i])
        if ok:
            break

    return ok;


# return value see vwtSalary
def vwtYuQing(phoneNum, password):
    ok = startVWT(phoneNum, password)
    if not ok: return (APP, '2-VWT Logon Error')

    wait("vwt-bar-work-1.png")
    click("vwt-bar-work-1.png")
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

    return (APP, '1')


# return '1': success
# return '2-V网通登录错误'
# return '3-不是薪酬通用户'
# return '4-未知错误'
def vwtSalary(phoneNum, failToKaoQin, password):
    ok = startVWT(phoneNum, password)
    if not ok: return (APP,'2-VWT Logon Error')

    time.sleep(0.7)
    wait("vwt-bar-work-1.png")
    click("vwt-bar-work-1.png")
    for x in range(0, 1):
        if exists("vwt-bar-work-1.png"):
            click("vwt-bar-work-1.png")
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

    app = APP
    if '1' != ret and failToKaoQin:
        click("vwt-salaryapp-close.png")
        time.sleep(0.3)
        click("android-back.png")
        wait("vwt-work-platform-title.png")
        r = find("vwt-work-platform-title.png").below(400)
        r.hover()

        wheel(WHEEL_DOWN, 4)
        click("vwt-kaoqin-app-icon.0.png")
        time.sleep(0.7)
        wait("vwt-kaoqin-mobile-kaoqin.png")
        click("vwt-kaoqin-loc-checkin.png")
        time.sleep(0.8)

        if exists("vwt-checkin-liketiyan.png"):
            click("vwt-checkin-liketiyan.png")

        click("vwt-kaoqin-checkin.png")
        time.sleep(0.5)
        click("vwt-kaoqin-checkin-confirm.png")
        time.sleep(0.5)
        app = "KaoQin"
        ret = '1'

        click("vwt-salaryapp-close.png")
        time.sleep(0.3)
        click("vwt-bar-me.png")
        logoutVWT(True)
        click("android-show-main.png")
        time.sleep(0.4)
        return (app, ret);

    click("vwt-salaryapp-close.png")
    time.sleep(0.3)
    click("android-back.png")
    time.sleep(0.5)
    click("vwt-bar-me.png")
    logoutVWT(True)
    click("android-show-main.png")
    time.sleep(0.4)
    return (app, ret);



# return '1': success
# return '2-V网通登录错误'
def vwtKaoQin(phoneNum, password):
    ok = startVWT(phoneNum, password)
    if not ok: return (APP, '2-VWT Logon Error')

    time.sleep(0.7)
    wait("vwt-bar-work-0.png")
    click("vwt-bar-work-0.png")
    for x in range(0, 1):
        if exists("vwt-bar-work-0.png"):
            click("vwt-bar-work-0.png")
            time.sleep(0.4)
        else:
            break

    if exists("vwt-work-i-see1.png"): 
        click("vwt-work-i-see1.png")
        time.sleep(0.2)
        if exists("vwt-work-i-see2.png"): click("vwt-work-i-see2.png")

    wait("vwt-work-platform-title.png")
    r = find("vwt-work-platform-title.png").below(300)
    r.hover()

    wheel(WHEEL_DOWN, 4)
    click("vwt-kaoqin-app-icon-0.png")
    time.sleep(1.5)
    wait("vwt-kaoqin-mobile-kaoqin.png")
    click("vwt-kaoqin-kaoqin-icon.png")
    time.sleep(0.5)
    if exists("vwt-checkin-liketiyan.png"):
        click("vwt-checkin-liketiyan.png")
        time.sleep(0.3)
    wait("vwt-kaoqin-title-kaoqin.png")

    punched = 0
    if exists("vwt-kaoqin-checkin-finger.png", 0):
        click("vwt-kaoqin-checkin-finger.png")
        punched = 1
    elif exists("vwt-kaoqin-checkout-finger.png", 0):
        click("vwt-kaoqin-checkout-finger.png")
        punched = 1
    elif exists("vwt-kaoqin-already-check.png", 0): 
        pass
    else:
        click("vwt-kaoqin-checkin-finger.png")
        punched = 1

    if (1 == punched):
        time.sleep(0.5)
        if exists("vwt-kq-already-checked.png", 0):
            click("vwt-kaoqin-confirm-btn.png")
            time.sleep(0.3)
        else:
            click("vwt-kqoqin-confirm-btn.png")
            time.sleep(0.5)
            if exists("vwt-kaoqin-confirm-text.png"):
                click("vwt-kaoqin-confirm-btn.png")
                time.sleep(1)

    click("vwt-salaryapp-close.png")
    time.sleep(0.5)
    click("vwt-bar-me.png")
    logoutVWT(True)
    click("android-show-main.png")
    time.sleep(0.4)
    return (APP, '1')


def doTask(phoneNum, MEID, password):
    # always changes MEID
    if len(MEID) < 10:
        setNoxMEID(phoneNum, "")
    else:
        setNoxMEID(phoneNum, MEID)

    if "Salary" == APP:
        return vwtSalary(phoneNum, True, password)
    elif "YuQing" == APP:
        return vwtYuQing(phoneNum, password)
    elif "KaoQin" == APP:
        return vwtKaoQin(phoneNum, password)

def restartAndroid():
    global NOX_PROCESS
    import subprocess
    print 'Enters restartAndroid()' 
    # workaround to fix the image identifying issue https://github.com/RaiMan/SikuliX-2014/issues/139
    Image.reset()

    for i in range(5):
        child = None
        try:
            try:
                if exists("nox-restart-confirm.png", 0):
                    click("nox-restart-confirm.png")
                if exists("close.png", 0):
                    click("close.png")

                clearToMain()
                click("nox-close.png")
                wait("nox-sure-to-close-emu.png")
                click("nox-restart-confirm.png")
                time.sleep(12)
            except FindFailed:
                pass

            NOX_PROCESS = child = subprocess.Popen([NOX_PATH, ''])
            time.sleep(20)
            if not exists("nox-simulator.png"):
                time.sleep(10)
                if not exists("nox-simulator.png"):
                    time.sleep(10)

            if exists("nox-restart-confirm.png"):
                click("nox-restart-confirm.png")
            clearToMain()
            return
        except FindFailed:
            if None != child:
                child.kill()
                time.sleep(1)
                child.terminate()
                time.sleep(1)
                child = None
            elif None != NOX_PROCESS:
                NOX_PROCESS.kill()
                time.sleep(1)
                NOX_PROCESS.terminate()
                time.sleep(1)
                NOX_PROCESS = None

def oneExcel(excelPath, dryRun):
    import xlrd
    import xlutils.copy
    global exceptionCount

    rb = xlrd.open_workbook(excelPath)
    rs = rb.sheet_by_index(0)
    wb = xlutils.copy.copy(rb)
    ws = wb.get_sheet(0)

    # assume it was successful
    exceptionCount = 0;
    for rownum in range(1, rs.nrows):
        phoneNum = str(rs.row_values(rownum)[0]).strip()
        if phoneNum.find('.') > 0:
            phoneNum = phoneNum.split('.')[0]
        if phoneNum == '': continue

        MEID = str(rs.row_values(rownum)[1])
        done = str(rs.row_values(rownum)[2])
        
        password = []
        try:
            for i in range(4, 10):
                s = str(rs.row_values(rownum)[i])
                if s.find('.') > 0:
                    s = s.split('.')[0]
                if s != '':
                   password.append(s)
        except IndexError:
            pass

        if dryRun == False and '' == done and len(password) != 0:
            print '[', excelPath, '] Task: ', rownum, ':', 'phoneNum =', phoneNum
            ret = ''

            if DEBUG == 1:
                (app, ret) = doTask(phoneNum, MEID, password)
                exceptionCount = 0
                ws.write(rownum, 2, ret)
                ws.write(rownum, 3, app)
                wb.save(os.path.join(getBundlePath(), excelPath))
            else:
                try:
                    (app, ret) = doTask(phoneNum, MEID, password)
                    exceptionCount = 0
                    ws.write(rownum, 2, ret)
                    ws.write(rownum, 3, app)
                    wb.save(os.path.join(getBundlePath(), excelPath))

                except FindFailed:
                    exceptionCount = exceptionCount + 1
                    print "exception FindFailed found, continuous count = ", exceptionCount

        if exceptionCount >= 3:
            restartAndroid()
            exceptionCount = 0
    return

def main(dryRun):
    import os
    global TASK_DATA_PATH

    pathes = []
    if not TASK_DATA_PATH.endswith("\\"):
        TASK_DATA_PATH = TASK_DATA_PATH + "\\"
    if TASK_DATA_PATH.startswith("data\\"):
        TASK_DATA_PATH = os.path.join(getBundlePath(), TASK_DATA_PATH)
    for file in os.listdir(TASK_DATA_PATH):
        if file.endswith(".xls"):
            pathes.append(os.path.join(TASK_DATA_PATH, file))

    for excelPath in pathes:
        print "excelPath: ", excelPath + ", dryRun: ", dryRun
        for k in range(5):
            oneExcel(excelPath, dryRun)

main(True)
main(False)
