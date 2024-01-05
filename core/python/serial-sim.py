import random
import time

random_number = random.choice([0, 1, 2, 3, 4, 998])
length = random.randint(1, 10)
for _ in range(length):
    print(random_number, flush=True)
    time.sleep(1)

print("#", flush=True)

while True:
    random_number = random.choice([0, 1, 2, 3, 4, 998])
    length = random.randint(1, 10)
    for _ in range(length):
        print(random_number, flush=True)
        time.sleep(1)