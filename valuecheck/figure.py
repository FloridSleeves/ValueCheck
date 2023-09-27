import matplotlib.pyplot as plt
import numpy as np

fig, ax = plt.subplots(figsize=(6, 3), subplot_kw=dict(aspect="equal"))

recipe = ["Network", "Memory", "Device", "FileSystem", "Security", "Others"]

data = [12,5,17,38,17,11]

wedges, texts, autotexts = ax.pie(data, labels=recipe, autopct='%1.0f%%', 
    #colors=['white'], 
    wedgeprops = {
        #"edgecolor" : "black",
        'linewidth': 1,
        'antialiased': True}, labeldistance=1.05)

plt.setp(texts, size=10, weight="normal")
plt.setp(autotexts, size=10, weight="normal", color='white')

#plt.show()
plt.savefig("result/figure_7_dist.pdf", format="pdf", bbox_inches="tight")

###################################

fig, ax = plt.subplots(figsize=(6, 3), subplot_kw=dict(aspect="equal"))

recipe = ["Low", "Medium", "High"]

data = [25,58,15]

wedges, texts, autotexts = ax.pie(data, labels=recipe, autopct='%1.0f%%', colors=['white'], wedgeprops = {"edgecolor" : "black",
                      'linewidth': 1,
                      'antialiased': True}, labeldistance=1.05)#autopct=lambda pct: func(pct, data), textprops=dict(color="w"))

plt.setp(texts, size=10, weight="normal")
plt.setp(autotexts, size=10, weight="normal")

#plt.show()
plt.savefig("result/figure_7_security.pdf", format="pdf", bbox_inches="tight")

###################################

data = [749, 430, 516, 5550, 514, 1820, 4551, 5423, 2415, 360, 2244, 469, 2637, 489, 3489, 3489, 3489, 229, 5428, 3020, 467, 1545, 2300, 2121, 396, 5905, 171, 1482, 5550, 5114, 6337, 3412, 3582, 3337, 749, 2984, 430, 2223, 1181, 4116, 6337, 6337, 6337, 516, 2559, 1179, 2462, 2462, 2838, 1125, 1125, 3478, 3478, 3478, 1743, 2230, 1714, 1714, 2559, 2559, 2559, 2230, 2488, 2769, 1265, 2769, 1265, 1265, 2769, 797, 2769, 2769, 7495, 2443, 1260, 789, 789, 1118, 2769, 1732, 829, 1659, 5682, 908, 1659, 1659, 1659, 3407, 416, 977, 977, 1659, 5421, 1176, 1148, 1659, 831, 2000, 1659, 1659, 1659, 1659, 837, 2344, 2056, 1659, 2599, 2599, 3952, 537, 1817, 1659, 1332, 1659, 1335, 1659, 1659, 1659, 1659, 1659, 1659, 451, 1659, 1659, 1659, 1658, 1659, 1659, 1659, 1659, 1659, 1659, 1370, 873, 1659, 1000, 1000, 588, 588, 1659, 1659, 1659, 475, 1659, 1659, 1659, 1659, 2054, 2140, 2054, 2054, 2054, 2054]
for i, d in enumerate(data):
    if d > 1100:
        data[i] = 1100

hist, bar = np.histogram(data, bins=11, range=(0,1100))

import matplotlib.pyplot as plt

fig, ax = plt.subplots(figsize=(5,1))

fruits = ["0", "100", "200", "300", "400", "500", "600", "700", "800", "900", "     1000+"]
counts = hist

ax.bar(fruits, counts, color='aquamarine', edgecolor='black', width=1, align='edge', hatch='//')

ax.set_ylabel('Bug Count', fontsize=10)
ax.set_xlabel('Days', fontsize=10)
ax.set_yticks([0,50,100], ['0', '50', '100'])
for label in ax.get_xticklabels(): 
    label.set_fontsize(10)

for label in ax.get_yticklabels(): 
    label.set_fontsize(10)

for i in range(len(fruits)):
    plt.text(i+0.5,hist[i]+5,hist[i], horizontalalignment='center')

ax.set_xlim([0,11])
ax.set_ylim([0,160])


#plt.show()
plt.savefig("result/figure_7_days.pdf", format="pdf", bbox_inches="tight")


import matplotlib.pyplot as plt
fig, ax = plt.subplots(figsize=(5,2))
x=[0,10,20,30,40]
y=[100,98,92,87,74]
ax.plot(x,y, color='black', marker='o', linestyle='solid', linewidth=2, markersize=5)
ax.set_ylabel('Precision', fontsize=15)
ax.set_xlabel('Reported Bugs from Each Application', fontsize=15)
ax.set_xticks([0,10,20,30,40], ['0', '10', '20','30','All'])
ax.set_yticks([70,80,90,100], ['70', '80', '90','100'])
ax.set_ylim([70,105])
ax.set_xlim([0,40])
for label in ax.get_xticklabels():
    label.set_fontsize(10)
for label in ax.get_yticklabels(): 
    label.set_fontsize(10)
for i in range(len(x)):
    plt.text(x[i]+0.5,y[i]+0.5,y[i], horizontalalignment='left')
plt.savefig("result/figure_9_detected_bug_dok.pdf", format="pdf", bbox_inches="tight")