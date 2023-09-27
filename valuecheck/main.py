#!/usr/bin/python
# coding=utf8
"""
# Author: lily
# Created Time : 2021-01-07 16:54:40

# File Name: main.py
# Description:

"""

import os
from utils import matchVar
from utils import matchLine
from utils import matchMarked
import re
from datetime import datetime

inline_flag = {'linux':True, 'nfs':False, 'ssl':True, 'mysql':False}# whether check inline
parse_flag = {'linux':True, 'nfs':False, 'ssl':False, 'mysql':False}#whether parse datetime

def executeOpt(bc_path, mid_file_name):
    print("20:")
    exit_code = os.system("opt -load /home/lily/HAPPY/LAB/apiusage/unused-analysis/Integration/LocalAnalysis/build/libLLVMLocalLivenessAnalysisPass.so -enable-new-pm=0 -LocalLiveness -t all < " + bc_path + " > /dev/null 2> " + mid_file_name)
    if exit_code != 0:
        # problem of execute
        print("Error executing 'opt'!")
        exit()

class UnusedObject:
    def __init__(self, fname, vname, path, lineno, loc):
        self.fname = fname
        self.vname = vname
        self.path = path
        self.lineno = lineno
        self.author_loc = loc
    
    def print(self):
        print(self.fname, '#', self.vname, '#', self.path, '#', self.lineno, '#', self.author_loc)

    def equal(self, item):
        return self.fname == item.fname and self.vname == item.vname and self.path == item.path and self.lineno == item.lineno
class Counter:
    def __init__(self):
        self.value_alias_counter = 0
        self.pointer_alias_counter = 0
        self.resource_cleanup_counter = 0
        self.inline_variable_counter = 0
        self.config_depend_counter = 0
        self.cursor_counter = 0
        self.marked_counter = 0
        self.final_count = 0

    def checkConfig(self, path, fname, vname):
        return matchVar(path, fname, vname)

    def checkMarked(self, path, fname, lineno):
        return matchMarked(path, fname, lineno)

    def checkCursor(self, path, fname, lineno):
        if lineno == -1:
            return False
        return matchLine(path, fname, lineno)

    def checkInline(self, path, fname, lineno, vname):
        try:
            f = open(path)
        except FileNotFoundError:
            return True
        try:
            lines = f.readlines()
        except UnicodeDecodeError:
            return False
        if len(lines) < lineno:
            return False
        if vname.startswith("*ret*"):
            vname = vname[5:]
        if inline_flag[sys.argv[4]] and lines[lineno-1].find(vname)  == -1:
            return True
        return False

    def checkExternal(self, path, fname, lineno, vname, prefix):
        if (path.find(".c") != -1 or path.find(".cpp") != -1 or path.find(".cc")!=-1 or path.find(".h") != -1) and path.find("yacc") == -1 and path.find(prefix) != -1:
            return self.checkInline(path, fname, lineno, vname)
        return True

    
    def print(self):
        print("[Statistics]")
        #print("Value Alias: ", self.value_alias_counter)
        #print("Pointer Alias: ", self.pointer_alias_counter)
        #print("Resource Cleanup: ", self.resource_cleanup_counter)
        #print("Inline Variable: ", self.inline_variable_counter)
        print("Config Dependent: ", self.config_depend_counter)
        print("Cursor: ", self.cursor_counter)
        print("Marked: ", self.marked_counter)
        print("Final Total: ", self.final_count)

def makeList(mid_file_name):
    f = open(mid_file_name, errors='ignore')
    fname = "" # which should not be used
    item_map = set()
    flag = False
    last_lineno = None
    last_path = None
    last_v = None
    for l in f.readlines():
        if l.startswith('error: Could not') or l[0] == ' ' or l[:3] in ['Mon']:
            continue
        if l.find('{') != -1:
            if not flag:
                continue
            flag = False
            lsplit = l.split('{')
            if len(lsplit) == 1:
                # no path, inline
                c.inline_variable_counter += 1
                continue
            vname = lsplit[0].split('[')[0][1:]
            try:
                path, lineno = lsplit[1].split(':')
            except ValueError:
                continue
            lineno = lineno[:-2]
            if len(lsplit) > 2:
                author_loc = "{" + "{".join(lsplit[2:])
            else:
                author_loc = ""
            if (not (path == last_path and lineno == last_lineno and vname == last_v) or fname == "") and (not lineno=="") :
                #print(" # ".join([fname, vname, path, lineno]))
                item_map.add(UnusedObject(fname, vname, path, int(lineno), author_loc.strip('\n')))
            last_path = path
            last_lineno = lineno
            last_v = vname
        elif (parse_flag[sys.argv[4]] and (l.find(':') != -1 and l[0] != '[')):
            flag = True
            fname = l[:-2] if l.find('$') == -1 else l.split('$')[-1][:-2]
            fname_flag = True
            try:
                dt = datetime.strptime(fname[:-3], "%a %d %b %Y %I:%M:%S %p")
            except ValueError as e:
                #print(e)
                fname_flag = False
            if fname_flag:
                fname = ""
            else:
                import cxxfilt
                fname = cxxfilt.demangle(fname)
                if fname.find("(") != -1:
                    pattern = r"([a-z_A-Z]+)\("
                    m = re.search(pattern, fname)
                    if m:
                        fname = m.group(1)
                    else:
                        #print("151:", fname)
                        fname = ""
                else:
                    fname = fname
        elif (not parse_flag[sys.argv[4]]) and l[-2] == ':' and l[0] != '[':
            flag = True
            fname = l[:-2] if l.find('$') == -1 else l.split('$')[-1][:-2]
            if sys.argv[4] == 'linux' or sys.argv[4] == 'mysql' or sys.argv[4] == 'ssl':
                fname_flag = True
                try:
                    dt = datetime.strptime(fname[:-3], "%a %d %b %Y %I:%M:%S %p")
                except ValueError as e:
                    #print(e)
                    fname_flag = False
                if fname_flag:
                    fname = ""
                else:
                    import cxxfilt
                    try:
                        fname = cxxfilt.demangle(fname)
                    except:
                        print('Cannot parse ', fname)
                    if fname.find("(") != -1:
                        pattern = r"([a-z_A-Z]+)\("
                        m = re.search(pattern, fname)
                        if m:
                            fname = m.group(1)
                        else:
                            #print("151:", fname)
                            fname = ""
                    else:
                        fname = fname
            elif sys.argv[4] == 'nfs':
                fname = fname

    f.close()
    return item_map


import sys
if __name__ == "__main__":
    item_set = set()
    for mid_file_name in sys.argv[3:-1]:
        item_map = makeList(mid_file_name)
        flag = False
        c = Counter()
        for item in item_map:
            #item.print()
            old = sys.argv[1] 
            new = sys.argv[2]
            item.path = item.path.replace(old, new)
            item.author_loc = item.author_loc.replace(old, new)
            if sys.argv[4] == 'ssl':
                item.path = item.path.replace('/build/../', '/')
                item.author_loc = item.author_loc.replace('/build/../', '/')
            if c.checkExternal(item.path, item.fname, item.lineno, item.vname, new):
                c.inline_variable_counter += 1
                continue
            if c.checkConfig(item.path, item.fname, item.vname):
                c.config_depend_counter += 1
                continue
            if c.checkCursor(item.path, item.fname, item.lineno):
                c.cursor_counter += 1
                continue
            if c.checkMarked(item.path, item.fname, item.lineno):
                c.marked_counter += 1
                continue
            if c.checkInline(item.path, item.fname, item.lineno, item.vname):
                continue
            flag = False
            for ent in item_set:
                if ent.equal(item):
                    flag = True
                    break
            if flag == False:
                item_set.add(item)
                item.print()
                c.final_count += 1
    c.print()            
