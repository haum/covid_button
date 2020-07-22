#!/usr/bin/env python3
import matplotlib.pyplot as plt
import csv, sys

d = {
    0: [],
    1: [],
    2: [],
    7: [],
}
with open(sys.argv[1] if len(sys.argv) > 1 else 'measures.txt') as csvfile:
  spamreader = csv.reader(csvfile, delimiter=',', quotechar='"')
  for row in spamreader:
      quality = int(row[0])
      value = int(row[1])
      for k in (0,1,2,7):
          if k == quality:
              d[k].append(value)
          else:
              d[k].append(-100)
x = tuple(range(len(d[0])))

plt.plot(x, d[0], 'o', label='Valide', color='#00cc00')
plt.plot(x, d[1], 'o', label='Sigma douteux', color='#ccff00')
plt.plot(x, d[2], 'o', label='Signal faible', color='#ffcc00')
plt.plot(x, d[7], 'ro', label='Vraiment chelou')
plt.figlegend(bbox_to_anchor=(0.25, 0.20))
plt.ylim(0, 1300)

plt.show()
