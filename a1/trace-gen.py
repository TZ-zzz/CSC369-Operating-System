from random import randrange

traceFile = "trace.txt"
profile = 3
# Easy: #0
if profile == 0:
    ops = 100
    min = -120
    max = 150

# Medium: #1
if profile == 1:
    ops = 100
    min = -800
    max = 1000

# Hard: #2
if profile == 2:
    ops = 500
    min = -800
    max = 1000

# Mark: #3
if profile == 3:
    ops = 10000
    min = -1900
    max = 2000

# open the file to write
f = open(traceFile,"w")
i = 0
# current number of items
total = 0
# highest number of items
total_max = 0
# set if the number of items ever reach zero and then pop/dequeue
total_minus = 0
def generate_trace(f, total, total_max, total_minus, local):
    i = 0
    while(i < ops):
        # generate a random number
        r = randrange(min,max,1)
        # skip if the number of items already reach zero and the generated number is minus
        if r < 0:
            if total == 0 or (total < 1000 and local == 1):
                continue
        total += r
        # never below zero
        if total < 0:
            total = 0
            total_minus = 1
        # update total_max
        if total > total_max:
            total_max = total
        # write to file 
        f.write(str(r)+"\n")
        i += 1
    return total,total_max,total_minus
total,total_max,total_minus = generate_trace(f, total, total_max, total_minus, 0)
# test around 1200 items for the purpose of worst # of malloc per 100 ops
if(profile == 3):
    min = -30
    max = 28
    # bring down to 1200
    f.write(str(-(total - 1200)) + "\n")
    total = 1200
    total, local_max, total_minus = generate_trace(f, total, 0, total_minus, 1)
    print("final total: " + str(total) + "\t\tlocal max: " + str(local_max))

print("max total: " + str(total_max) + "\t\tempty tested: " + str(total_minus))

f.close()
