from matplotlib import pyplot as plot

MOTOR_A = 21
MOTOR_B = 18

WINDOW = 15

if __name__ == "__main__":
    motor_a_avg = 16.0
    motor_b_avg = 9.0

    motor_a_avg_list = []
    motor_b_avg_list = []

    with open("motor_ab.dat", "r") as f:
        for line in f:
            motor_neuron_status = [int(x) for x in line.split(" ")]
            motor_a_sum = 100.0 * sum(motor_neuron_status[:MOTOR_A]) / float(MOTOR_A)
            motor_b_sum = 100.0 * sum(motor_neuron_status[MOTOR_A:]) / float(MOTOR_B)

            motor_a_avg = (motor_a_sum + WINDOW*motor_a_avg) / (WINDOW + 1)
            motor_b_avg = (motor_b_sum + WINDOW*motor_b_avg) / (WINDOW + 1)

            motor_a_avg_list.append(motor_a_avg)
            motor_b_avg_list.append(motor_b_avg)

    x = range(len(motor_a_avg_list))

    plot.plot(x, motor_a_avg_list, '-r', label='A-type motor neurons')
    plot.plot(x, motor_b_avg_list, '-b', label='B-type motor neurons')

    plot.axhline(19.0, linestyle='--', color='r', label='Robot reverse threshold')

    plot.title("Nanotode light-weight C. elegans simulation\n\nMotor neuron response to sensory input\n(Nose touch t > 1000)")
    plot.xlabel("t [neural sim ticks]")
    plot.ylabel("avg % neurons firing ({} tick window)".format(WINDOW))

    plot.legend(loc=2)
    
    plot.tight_layout()

    plot.savefig('motor_avg.png')
