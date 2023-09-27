#!/usr/bin/python
# coding=utf8
"""
# Author: lily
# Created Time : 2020-11-03 09:07:27

# File Name: extract.py
# Description:
    utils for extracting function from a file

"""
import re
import sys

def extractFunc(fname, func):
    try:
        f = open(fname, 'r', errors='ignore')
    except FileNotFoundError:
        return ""
    find_name = False
    in_func = False
    func_body = ""
    pat = re.escape(func)+"[' ']*\("
    for l in f.readlines():
        res = re.search(pat, l)
        if find_name == False and res != None and l.find(';') == -1:
            find_name = True
        if find_name and in_func == False and l.find('{') != -1:
            in_func = True
            continue
        if find_name and not in_func and l.find(';') != -1:
            find_name = False
            in_func = False
            continue
        if in_func and l[0] == '}':
            break
        if in_func:
            func_body += l
            continue
    f.close()
    return func_body

def matchVar(fname, func, vname):
    func_body = extractFunc(fname, func)
    results = re.findall(r"#if[\S\ \n\t]+#endif", func_body)
    flag = False
    for res in results:
        pat = '([^a-zA-Z_]|^)'+vname+'[^a-zA-Z_]'
        result = re.findall(pat, res)
        if len(result) > 0:
            flag = True
            break
    return flag

def findall(string, substr):
    start = 0
    while True:
        start = string.find(substr, start)
        if start == -1:
            return
        yield start
        start = start + len(substr)

def matchLine(fname, func, lineno):
    func_body = extractFunc(fname, func)
    try:
        f = open(fname, errors='ignore')
    except FileNotFoundError:
        return False
    lines = f.readlines()
    f.close()
    if len(lines) < lineno:
        return False
    pat = lines[lineno-1].strip()
    if len(pat) == 0:
        return True
    result = list(findall(func_body, pat))
    return len(result) > 1 or pat.find("++") != -1 or pat.find("l2n") != -1 or pat.find("n2l") != -1 or pat.find("l2c") != -1 or pat.find("c2l") != -1

def matchMarked(fname, func, lineno):
    func_body = extractFunc(fname, func)
    try:
        f = open(fname, errors='ignore')
    except FileNotFoundError:
        return False

    lines = f.readlines()
    f.close()
    if len(lines) < lineno:
        return False
    pat = lines[lineno-1].strip()
    return pat.lower().find("unused") != -1


def test():
    return matchVar('/home/lily/lab/apiusage/squid/src/icmp/net_db.cc', 'netdbHostRtt', 'host')

