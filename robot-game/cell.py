from dataclasses import dataclass


@dataclass
class Cell:
    top: bool = False
    right: bool = False
    down: bool = False
    left: bool = False
    blocked: bool = False

    def to_json(self):
        return {
            "top": self.top,
            "right": self.right,
            "down": self.down,
            "left": self.left,
        }