import sys, os
from math import log
f=open(sys.argv[1])
lines=f.readlines()
DOK_1 = []
DOK_2 = []
DOK_3 = []
for l in lines:
    tmp = l.split('#')
    DOK_1.append(int(tmp[-3]))
    DOK_2.append(int(tmp[-2]))
    DOK_3.append(int(tmp[-1]))

DOK_1_max = max(DOK_1)
DOK_2_max = max(DOK_2)
DOK_3_max = max(DOK_3)

for i in range(len(DOK_1)):
    DOK_1[i] += 1
DOK_1 = [x/DOK_1_max for x in DOK_1]
DOK_2 = [x/DOK_2_max for x in DOK_2]
DOK_3 = [x/DOK_3_max for x in DOK_3]

DOK = []
DOK_REMOVE_1 = []
DOK_REMOVE_2 = []
DOK_REMOVE_3 = []

for i in range(len(DOK_1)):
    DOK.append(3.1+1.2*DOK_3[i] + 0.2*DOK_2[i]-0.5*log(DOK_1[i],10))
    DOK_REMOVE_1.append(DOK[i]-0.5*log(DOK_1[i], 10))
    DOK_REMOVE_2.append(DOK[i]- 0.2*DOK_2[i])
    DOK_REMOVE_3.append(DOK[i]- 1.2*DOK_3[i])

check_rank = int(sys.argv[2])
DOK_rank = [sorted(DOK).index(x) for x in DOK]
DOK_REMOVE_1_rank = [sorted(DOK_REMOVE_1).index(x) for x in DOK_REMOVE_1]
DOK_REMOVE_2_rank = [sorted(DOK_REMOVE_2).index(x) for x in DOK_REMOVE_2]
DOK_REMOVE_3_rank = [sorted(DOK_REMOVE_3).index(x) for x in DOK_REMOVE_3]

count1 = 0
count2 = 0
count3 = 0

for i in range(len(DOK_rank)):
    if DOK_rank[i] < check_rank:
        #print(lines[DOK_rank[i]], end='')
        if DOK_REMOVE_1_rank[i] < check_rank and count1 < check_rank:
            count1 += 1
        if DOK_REMOVE_2_rank[i] < check_rank and count2 < check_rank:
            count2 += 1
        if DOK_REMOVE_3_rank[i] < check_rank and count3 < check_rank:
            count3 += 1
app = os.path.basename(sys.argv[1]).replace('-with-author', '') 
os.makedirs("result/" + app, exist_ok=True)
fout = open('result/' + app + "/detected.csv", 'w')
fout.write('function # var # path # lineno # callsite (optional) # time # AC # DL # FA # Rank\n')
for i in range(len(DOK_rank)):
    fout.write(lines[i].strip() + ' # ' + str(DOK_rank[i]) + '\n')
fout.close()
fout=open('result/table_2_detected_bugs.csv', 'a')
fout.write(app + ", " + str(len(DOK_rank)) + "\n")
fout.close()
print(", ".join([app, str(check_rank), str(count1), str(count3), str(count2)]))
