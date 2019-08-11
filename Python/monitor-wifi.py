#https://stackoverflow.com/questions/14077835/associating-my-windows-computer-to-a-wifi-ap-with-python
import subprocess

ADAPERS = [
    ['48-8A-D2-75-12-60', 'aWiFi'],
    ['50-89-65-01-C3-52', 'SmartOffice-5G'],
    ['48-8A-D2-E0-9C-21', 'SAP-Internet']
]


def disconnect_wlan():
    process = subprocess.Popen('netsh wlan disconnect', shell=True,
        stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    stdout, stderr = process.communicate()
    stdout = stdout.decode("utf-8")

    # Return `True` if we were able to successfully connect
    return ('successfully' in stdout) or ('\xd2\xd1\xb3\xc9\xb9\xa6' in stdout) #已成功...


def connect_to_network(interface, ssid):
    cmd = 'netsh wlan connect name={s} ssid={s} interface="{i}"'.format(i=interface, s=ssid)
    print(cmd)
    process = subprocess.Popen(
        cmd,
        shell=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE)
    stdout, stderr = process.communicate()
    stdout = stdout.decode("utf-8")
    print(stdout, stderr)

    # Return `True` if we were able to successfully connect
    return ('successfully' in stdout) or ('\xd2\xd1\xb3\xc9\xb9\xa6' in stdout) #已成功...


def connected_ssids():
    process = subprocess.Popen(
        'netsh wlan show interfaces',
        shell=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE)
    stdout, stderr = process.communicate()
    stdout = stdout.decode("utf-8")
    #print(stdout, stderr)

    connected = []

    name = ''
    ssid = ''
    mac = ''
    for s in stdout.split('\r\n'):
        s = s.strip()
        subs = s.split(':')
        if len(subs) == 2 and subs[0].strip() == 'Name':
            name = subs[1].strip()
        if len(subs) > 2 and subs[0].strip() == 'Physical address':
            mac = s[s.find(':') + 1:].strip().upper().replace(':', '-')
        if len(subs) == 2 and subs[0].strip() == 'SSID':
            ssid = subs[1].strip()
            connected.append([name, ssid, mac])
            name = ''
            ssid = ''
            mac = ''

    return connected


def available_ssid():
    process = subprocess.Popen(
        'netsh wlan show networks',
        shell=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE)
    stdout, stderr = process.communicate()
    stdout = stdout.decode("utf-8")
    #print(stdout, stderr)

    avai_ssids = []
    for s in stdout.split('\r\n'):
        s = s.strip()
        if s.startswith( 'SSID ' ):
            subs = s.split(':')
            if len(subs) == 2:
                avai_ssids.append(subs[1].strip())

    return avai_ssids


# check if the connected SSID are matching the MAC address defined in ADAPERS
def check_connected_ssids_valid(connected_ssids):
    tuple_list = []
    for e in connected_ssids:
        tuple_list.append((e[1], e[2])) # (ssid, mac)

    result = True
    for adapter in ADAPERS:
        if (adapter[1], adapter[0]) in tuple_list:
            adapter.append(True)
        else:
            adapter.append(False)
            result = False

    return result


def get_mac_to_wifi_adapter_dict():
    process = subprocess.Popen('ipconfig /all', shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    stdout, stderr = process.communicate()
    stdout = stdout.replace((0xc4).to_bytes(1, 'little'), ' '.encode('utf-8'))
    stdout = stdout.replace((0xea).to_bytes(1, 'little'), ' '.encode('utf-8'))
    stdout = stdout.replace((0xd4).to_bytes(1, 'little'), ' '.encode('utf-8'))
    stdout = stdout.replace((0xc2).to_bytes(1, 'little'), ' '.encode('utf-8'))
    stdout = stdout.replace((0xc8).to_bytes(1, 'little'), ' '.encode('utf-8'))
    stdout = stdout.replace((0xd5).to_bytes(1, 'little'), ' '.encode('utf-8'))
    stdout = stdout.decode("utf-8")
    #print(stdout, stderr)

    result = {}
    adapter = ''
    mac = ''
    for s in stdout.split('\r\n'):
        s = s.strip()
        subs = s.split(':')
        if len(subs) == 2 and subs[0].startswith('Wireless LAN adapter ') and subs[1].strip() == '':
            adapter = subs[0][len('Wireless LAN adapter ') : ].strip()
        if len(subs) == 2 and subs[0].strip().startswith('Physical Address'):
            mac = subs[1].strip().upper()
            if (adapter != ''):
                result[mac] = adapter
                adapter = ''
                mac = ''

    return result


#from __future__ import print_function
ssids = connected_ssids()
print('Connected SSIDs:')
for (adapter, ssid, mac) in ssids:
    print('    %s (%s): %s' %(adapter, mac, ssid))

if False == check_connected_ssids_valid(ssids):
    mac2adapter = get_mac_to_wifi_adapter_dict()

    for adapter in ADAPERS:
        if adapter[2] == False:
            mac, ssid = adapter[0], adapter[1]
            print('Device "%s" is not connnected to "%s". Start connecting ...' %(mac, ssid))
            try:
                adapter = mac2adapter[mac]
            except KeyError:
                print('Device "%s" is not found!' %(mac))
                continue
            connect_to_network(adapter, ssid)
