### 文件夹里面文件都是什么东西

```mermaid
HCI-hand_gesture_classify
├── data_process ：里面处理数据，不用管
└── model
    └── point_history_classifier
        ├── train_data ：训练数据
        ├── hand_classifier_v2.tflite ：直接用的inference模型，如果这个模型不行只替换它就可以
        ├── point_history_classifier.py ：里面定义了调用推断
        ├── training_hand_addnorm+2lstm.hdf5 ：格式转换前的模型，不用管
        └── __init__.py
├── utils
└── readme.md

```

### YOLO后数据的处理方法：

1. **为什么处理**：从YOLO出来的数据都是(0,1)的数据，我们把每一帧的手放缩成一样的大小，这个大小是将手腕到小拇指手掌连接处的距离作为单位1，对其余的坐标点进行放缩。
2. **如何处理**：
    ```python
    def normalize_hand_size(row, dimension=42):
        # reshape to array 这个3需要根据yolo输出的格式进行修改
        # 应该设置成0就可以，如果YOLO没有其他的输出的话
        points = row[0:].values.reshape(target_steps, dimension)

        # Step 1: Normalize first keypoint to (0,0) 
        # adjust other keypoints accordingly
        ref_point = points[:2] 
        for i in range(0, dimension, 2):
            points[i:i+2] -= ref_point

        # Step 2: Scale based on the distance between keypoints 0 and 17 
        scale_factor = np.linalg.norm(points[34:36])
        points /= scale_factor

        return points.flatten()
    ```
   - 上面的代码插到YOLO识别完后返回一帧的左边那里，调用一个它。
3. **可能还要处理的**：YOLO的帧率，这个得根据识别的结果来看能不能成。

### 模型概览

- 我们要把模型放在：与总inference同一级的目录下建一个model/，里面放我们的模型
- 模型输入：
  - model_path
  - score_th：下限，分类出来如果没有一类概率达到0.95以上输出998，说明是一个无效的手势

### 模型调用方法

1. 先实例化一个：
    ```python
    classifier = PointHistoryClassifier(model_path='model/point_history_classifier/hand_classifier_v2.tflite', score_th=0.95)
    ```
2. 放pipeline里头：
    ```python
    hand_point_buffer = [] 
    TIME_STEPS = 23
    DIMENSION = 42
    for frame in video_stream:
        #决定YOLO采样不采样这个帧
        
        #YOLO获得当前帧

        #YOLO完用上面的处理当前帧
        normalized_data = df.apply(lambda row: normalize_hand_size(row), axis=1)

        #把这一帧和之前获得的22帧并到一起，作为hand_point
        hand_point_buffer.append(normalized_data)
        if len(hand_point_buffer) > TIME_STEPS * DIMENSION:
            hand_point_buffer = hand_point_buffer[DIMENSION:]

        # 当23帧时进行手势识别，分类
        if len(hand_point_buffer) == TIME_STEPS * DIMENSION:
            gesture_id = classifier(hand_point_buffer)
    
    #进行下一步
    ```
