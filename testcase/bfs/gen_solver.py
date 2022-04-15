from numpy import random

f = open("bfs_kernel.log", mode = 'r')
output = open("bfs_inter_940_solver.log", mode = 'w')
random_file = open("random_940.log", mode = 'r')
random = []
t = random_file.readlines()
for line in t:
    number = int(line)
    random.append(number)
total = f.readlines()
res = 0.0
j = 0
iteration = []
for line in total : 
    string = line.split('  ')
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
idle = "0 " + str(random[0]) + "\n"
output.write(idle)

for i in range(1, 10) :
    for it in iteration :
        output.write(it)
    idle = "0 " + str(random[i]) + "\n"
    output.write(idle)
f.close()
output.close()
print(res)
print(j)