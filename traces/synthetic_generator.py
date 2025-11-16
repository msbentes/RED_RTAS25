import csv
import random

with open("synthetic.csv","w") as f:
    w = csv.writer(f)
    for i in range(1000):
        r = i * 5
        e = random.randint(1,5)
        d = r + random.randint(5,20)
        w.writerow([i,r,e,d])

