import random, math, sys


def create(out):
    arr = []
    for _ in range(500):
        num = int(random.random() * 1000)
        arr.append(str(num))
        arr.append(str(-num))

    with open(out, 'w') as file:
        file.write('\n'.join(arr))


if __name__ == '__main__':
    create(sys.argv[1])
