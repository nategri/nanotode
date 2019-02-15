from matplotlib import pyplot

f = open("motor_ab.dat")

a_t = []
b_t = []
a_data = []
b_data = []

tick = 0
for line in f:
    states = [int(x) for x in line.split()]
    a_states = [-1*(i+1) for i in range(len(states[:21])) if states[:21][i] > 0]
    b_states = [(i+1) for i in range(len(states[21:])) if states[21:][i] > 0]

    a_data += a_states
    b_data += b_states
    a_t += [tick for i in range(len(a_states))]
    b_t += [tick for i in range(len(b_states))]

    tick += 1

pyplot.plot(a_t, a_data, 'ro')
pyplot.plot(b_t, b_data, 'bo')

pyplot.savefig("motor_ab.png")
