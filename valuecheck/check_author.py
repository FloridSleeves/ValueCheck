import sys
import git
import sys
from git import Repo
from git import exc
import numpy as np
import fam_utils

range_flag = {'linux':False, 'nfs':True, 'mysql':False, 'ssl':False}

def getFamScore(filename, lineno, new, old, repo):
    old_filename = filename.replace("apiusage/"+new+"/", "apiusage/"+old+"/").strip()
    linecount = 0
    try:
        blame_res = repo.blame('HEAD', old_filename)
    except exc.GitCommandError as e:
        #print("22:", e)
        return 10000
    for commit, l_content in blame_res:
        linecount += len(l_content)
        if linecount >= lineno:
            author = commit.author.name
            FA, DL, AC = fam_utils.calMetric(author, old_filename, repo, commit)
            DOK = 3.1 + 1.2*FA + 0.2*DL - 0.5*np.log(1+AC)
            DOK_AC = AC#3.1 + 0.2*DL - 0.5*np.log(1+AC)
            DOK_DL = 1 if DL == 0 else DL #3.1 + 1.2*FA - 0.5*np.log(1+AC)
            DOK_FA = FA #3.1 + 1.2*FA + 0.2*DL
            return map(str, [DOK_AC, DOK_DL, DOK_FA])
    return 10000

if __name__ == "__main__":
    mid_file_name = sys.argv[1]
    repo_path = sys.argv[2] + "/"
    repo = git.Repo(repo_path)
    with open(mid_file_name, 'r') as f:
        for l in f.readlines():
            if l[0] == '[':
                break
            try:
                func, _, path, lineno, author_loc = l.split(' # ')
                #print(l)
                if func.split(' ')[0] in ['Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat', 'Sun']:
                    func = ""
                path = path.replace(repo_path, '')
                lineno = int(lineno)
                # identify author a
                authorA = None
                timeA = None
                blamelist = repo.blame_incremental('HEAD', path)
                for b in blamelist:
                    r = b.linenos
                    if range_flag[sys.argv[3]]:
                        r = range(r.start, r.stop+1)
                    if lineno in r:
                        authorA = b.commit.author
                        timeA = b.commit.authored_datetime
                        break
                # identify author b
                authorB = None
                if author_loc != '\n':
                    path, lineno = author_loc.lstrip('{').strip('}\n').split(':')
                    lineno = int(lineno)
                    path = path.replace(repo_path, '')
                    blamelist = repo.blame_incremental('HEAD', path)
                    for b in blamelist:
                        if lineno in b.linenos:
                            authorB = b.commit.author
                            break
                if authorA is None or authorB is None or authorA.name != authorB.name:
                    dok = getFamScore(path, lineno, "new", "old", repo)
                    print(l.strip('\n'), "#", timeA, '#', " # ".join(dok))
            except Exception as e:
                continue
