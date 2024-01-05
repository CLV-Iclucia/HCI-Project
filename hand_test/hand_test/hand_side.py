from point_history_classifier import PointHistoryClassifier
import numpy as np
import serial
from time import sleep

classifier = PointHistoryClassifier(model_path='model/point_history_classifier/hand_classifier_v2.tflite', score_th=0.95)


def normalize_hand_size(row, target_steps=1, dimension=42):
    # reshape to array 这个3需要根据yolo输出的格式进行修改
    # 应该设置成0就可以，如果YOLO没有其他的输出的话
    # points = row[0:].values.reshape(target_steps, dimension)
    points = np.array(row)

    # Step 1: Normalize first keypoint to (0,0)
    # adjust other keypoints accordingly
    ref_point = points[:2]
    for i in range(0, dimension, 2):
        points[i:i+2] -= ref_point

    # Step 2: Scale based on the distance between keypoints 0 and 17
    scale_factor = np.linalg.norm(points[34:36])
    points /= scale_factor

    return points.flatten()


hascom = False
while not hascom:
    hascom = False
    try:
        ser = serial.Serial("COM3", 115200, timeout=5)
        hascom = True
    except serial.serialutil.SerialException:
        print("COM3 Not Found")
        sleep(1)

print("COM3 Found")

hand_point_buffer = []
TIME_STEPS = 23
DIMENSION = 42
cnt = 0

print("#", flush=True)

while True:
    # 决定YOLO采样不采样这个帧
    cnt += 1

    # YOLO获得当前帧
    ch = ''
    buf = ''
    row = []
    while ser.read(1) != bytes('[', encoding='utf-8'):
        pass
    while ch != ']':
        try:
            ch = ser.read(1).decode()
        except UnicodeDecodeError:
            continue
        if ch == '[':
            continue
        if ch == ',':
            row.append(float(buf))
            buf = ''
        else:
            buf += ch
    row.append(float(buf[:-1]))

    # print(row)

    # YOLO完用上面的处理当前帧
    normalized_data = normalize_hand_size(row)
    # print(normalized_data)

    # 把这一帧和之前获得的22帧并到一起，作为hand_point
    hand_point_buffer.extend(normalized_data)
    # print(len(hand_point_buffer))
    if len(hand_point_buffer) > TIME_STEPS * DIMENSION:
        hand_point_buffer = hand_point_buffer[DIMENSION:]

    # 当23帧时进行手势识别，分类
    if len(hand_point_buffer) == TIME_STEPS * DIMENSION:
        gesture_id = classifier(hand_point_buffer)
        if gesture_id != 998:
            print(str(gesture_id), flush=True)
