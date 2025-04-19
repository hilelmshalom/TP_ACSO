for i in range(16):
    for j in range(-17,1):
        exclusive = i ^ j
        answer = exclusive // 2
        print(f"i: {i}, j: {j}, exclusive: {exclusive}, {bin(exclusive)}, answer: {answer}, {bin(answer)}")