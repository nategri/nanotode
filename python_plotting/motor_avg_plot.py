from matplotlib import pyplot

x = []
y = []

i = 0

with open('motor_avg', 'r') as f:
    for line in f:
        x.append(i)
        y.append(float(line))
        i += 1
        
pyplot.plot(x, y, 'b-')

pyplot.savefig("motor_avg.png")
