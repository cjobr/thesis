from numpy import random

f = open("kernel.log", mode = 'r')
output = open("detectron_100ms_inter_100.log", mode = 'w')

x = random.poisson(lam=100, size=20)
total = f.readlines()
res = 0.0
j = 0
iteration = []
for line in total : 
    string = line.split('  ')
    if(string[0] == "17.2983s"): 
        break
    j = j + 1
    temp = string[1]
    number = float(temp[:-2])
    if(temp[-2:] == "us") : number = number / 1000 
    res += number
    if(res >= 100) : 
        result = "1 " + str(res) + "\n"
        output.write(result)
        iteration.append(result) 
        print(result)
        res = 0
if res > 0 :
    result = "1 " + str(res) + "\n"
    output.write(result)
    iteration.append(result) 
idle = "0 " + str(x[0]) + "\n"
output.write(idle)

for i in range(1, 10) :
    for it in iteration :
        output.write(it)
    idle = "0 " + str(x[i]) + "\n"
    output.write(idle)
f.close()
output.close()
print(res)
print(j)
