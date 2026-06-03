class Robot:
    def __init__(self, x: int, y: int):
        self.start_x = x
        self.start_y = y

        self.x = x
        self.y = y

        self.crashed = False
        self.finished = False

    def reset(self):
        self.x = self.start_x
        self.y = self.start_y
        self.crashed = False
        self.finished = False