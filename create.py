import random, math, sys


def create(out):
    arr = []
    for i in range(499, -1, -1):
        arr.append(str(i))
        arr.append(str(-i))


    with open(out, 'w') as file:
        file.write('\n'.join(arr))


if __name__ == '__main__':
    create(sys.argv[1])
