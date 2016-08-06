#!/bin/sh

magic='--calling-python-from-/bin/sh--'
"""exec" python -E "$0" "$@" """#$magic"


# Specify the repo list you do NOT want to link them to common project folder.
# Typically, they are the repositories to which you often makes changes.
# The path name can be found in .repo/manifest.xml
EXCLUDED_PROJECT_LIST = ("packages/apps/Settings", "motorola/packages/blur")

# Specify the path where to store the shared projects. Must be absolute path.
SHARED_PROJECTS_PATH = '/localrepo/<core-id>/shared_projects'


import os
import sys
from xml.etree import ElementTree


def printUsage():
    print "More usage details, see https://sites.google.com/a/motorola.com/yingdai/shared-sync"


# Make sure the common shared folder is created
def createSharedProjsFolder(xml_projects):
    if not os.path.exists(getSharedProjsPath()):
        os.makedirs(getSharedProjsPath())
    for project in xml_projects:
        project_path = getSharedProjsPath() + project.attrib['path'] + '.git'
        if not os.path.exists(project_path):
            # make sure parent path is existing
            parent = getParentPath(project_path)
            if not os.path.exists(parent):
                os.makedirs(parent)
            # os.system('cd ' + parent + '; git init -q; ' + 'mv ' + parent + '/.git ' + project_path)
            os.system('git init -q ' + parent + '; mv ' + parent + '/.git ' + project_path)
            print 'Created ' + project_path


# create .repo/projects
def createPorjsFolder(xml_projects):
    projs_folder = '.repo/projects/'
    if not os.path.exists(projs_folder):
        os.makedirs(projs_folder)

    for project in xml_projects:
        if project.attrib['path'] not in EXCLUDED_PROJECT_LIST:
            proj_path = projs_folder + project.attrib['path'] + '.git'
            source = getSharedProjsPath() + project.attrib['path'] + '.git'
            if not os.path.exists(proj_path):
                # make sure parent path exists
                parent_path = getParentPath(proj_path)
                if not os.path.exists(parent_path):
                    os.makedirs(parent_path)
                os.symlink(source, proj_path)


def getParentPath(strPath):
    if not strPath:
        return None;
    lsPath = os.path.split(strPath);
    if lsPath[1]:
        return lsPath[0];
    lsPath = os.path.split(lsPath[0]);
    return lsPath[0];


def getProjNameByPath(xml_projects, path):
    for project in xml_projects:
        if project.attrib['path'] == path:
            return project.attrib['name']
    return None


gSharedProjectsPath = None
def getSharedProjsPath():
    global gSharedProjectsPath

    if gSharedProjectsPath == None:
        if SHARED_PROJECTS_PATH == '/localrepo/<core-id>/shared_projects' or \
        SHARED_PROJECTS_PATH == None or \
        SHARED_PROJECTS_PATH == '':
            print 'Error: invalid SHARED_PROJECTS_PATH! Update SHARED_PROJECTS_PATH in the script!'
            printUsage()
            sys.exit(1)

        gSharedProjectsPath = SHARED_PROJECTS_PATH
        if gSharedProjectsPath[-1] != '/':
            gSharedProjectsPath += '/'
    return gSharedProjectsPath


class InvalidProjectsPathError(Exception):
    """Invalid shared projects path.
    """


if __name__=="__main__":
    manifest = '.repo/manifest.xml'
    if not os.path.exists(manifest):
        print 'Error: no valid .repo folder found. Need to run "repo init" first!'
        printUsage()
        sys.exit(1)

    manifest_root = ElementTree.parse(manifest)
    xml_projects = manifest_root.getiterator("project")

    createSharedProjsFolder(xml_projects)
    createPorjsFolder(xml_projects)
