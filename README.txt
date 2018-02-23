
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
