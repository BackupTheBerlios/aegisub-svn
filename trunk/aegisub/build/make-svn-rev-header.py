#!/usr/bin/env python
# This file is used to automatically extract the SVN revision number and
# create a C include file.

# In MSVS, you can add the following Pre-Build Step to use this script:
#   cd $(InputDir)core\build
#   c:\Python24\python.exe make-svn-rev-header.py

# This is intended for use with a custom version.cpp,
# as described in version.cpp

from xml.dom.minidom import parse
from time import gmtime, strftime
from os.path import isfile
from sys import exit

entries_file = parse('../.svn/entries')

def getRevision(entries):
    for entry in entries:
        if entry.getAttribute("name") == "":
            return entry.getAttribute("revision")

revision = getRevision(entries_file.getElementsByTagName("entry"))

if isfile("svn-revision.h"):
    infile = file("svn-revision.h", "r")
    for ln in infile:
        if ln == "#define BUILD_SVN_REVISION " + revision + "\n":
            infile.close()
            exit()

outfile = file("svn-revision.h", "w+")
outfile.write("// This file is automatically generated by make-svn-rev-header.py\n")
outfile.write("// Do not modify or add to revision control\n\n")
outfile.write("#define BUILD_SVN_REVISION " + revision + "\n")
outfile.close()
