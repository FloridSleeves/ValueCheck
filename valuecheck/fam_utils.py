#!/usr/bin/python
# coding=utf8
"""
# Author: lily
# Created Time : 2021-01-31 00:49:45

# File Name: utils.py
# Description:

"""
from git import Repo
from git import exc
import numpy as np

def calMetric(author, filename, repo, line_commit):
    commits = repo.iter_commits('--all', paths = filename)
    nfs_flag = filename.find('nfs-ganesha') != -1
    first_author = None
    first_author_flag = 0
    dl_count = 0
    ac_count = 0
    for c in commits:
        first_author = c.author.name
        if c.author.name == author:
            dl_count += 1
        else:
            ac_count += 1
        if (not nfs_flag) and c == line_commit:
            break

    if first_author == author:
        first_author_flag = 1
    return (first_author_flag, dl_count, ac_count)


