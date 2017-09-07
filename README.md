# Clone repo
====================================================================================================
bash
$ cd /cygdrive/d/Dev/MyProgram (or some other location)
$ git clone http://code.google.com/p/eagle-util/

Add following line to ~/.bashrc
alias eagle-util='cd /cygdrive/d/Dev/MyProgram/eagle-util'

# .git/config
====================================================================================================
[core]
    filemode = false
[user]
	name = dynuaa
	email = dynuaa@gmail.com
[http]
	proxy = 192.168.1.200:8087
    sslVerify = false

url = http://code.google.com/p/eagle-util/ 
==> 
url = https://code.google.com/p/eagle-util/

To auto push without prompt of password:
url = https://<username>:<password>@code.google.com/p/<project>/


Proxy can be set by:
git config http.proxy 192.168.1.200:8087
git config http.proxy 127.0.0.1:8087

For empty git repo, need to run below firstly
git push origin master

# git commands:
---------------------------------------------------------------------------------------------------
git pull http://code.google.com/p/eagle-util/

git checkout -b development
git pull development:development        // remote development branch to localbranch

git checkout -b development
git add .
git commit -m "some brief comments"
git commit --amend

git push origin development:master     // local branch submit to remote master branch

git push origin development            // local branch submit to remote branch with same name
git push origin :my_branch1            // delete remote my_branch1

git tag
git tag -a HSS7-003 -m "some comments"
git push --tags                        // also push tags

git format-patch -2                    // two patch files
git reset --hard HEAD~2                // remove pervious 2 patches
