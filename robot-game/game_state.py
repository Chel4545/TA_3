from maze import Maze
from robot import Robot


class GameState:
    def __init__(self, maze: Maze, robot: Robot):
        self.maze = maze
        self.robot = robot

    def status(self) -> str:
        if self.robot.crashed:
            return "crashed"

        if self.robot.finished:
            return "finished"

        return "running"

    def move(self, direction: str):
        if self.robot.crashed:
            return {
                "ok": False,
                "error": "robot_already_crashed",
                "status": self.status(),
            }

        if self.robot.finished:
            return {
                "ok": False,
                "error": "robot_already_finished",
                "status": self.status(),
            }

        direction_map = {
            "top": {
                "dx": 0,
                "dy": -1,
                "wall": "top",
            },
            "bottom": {
                "dx": 0,
                "dy": 1,
                "wall": "down",
            },
            "left": {
                "dx": -1,
                "dy": 0,
                "wall": "left",
            },
            "right": {
                "dx": 1,
                "dy": 0,
                "wall": "right",
            },
        }

        if direction not in direction_map:
            return {
                "ok": False,
                "error": "unknown_direction",
                "status": self.status(),
            }

        current_cell = self.maze.get_cell(self.robot.x, self.robot.y)

        move_info = direction_map[direction]
        wall_name = move_info["wall"]

        if getattr(current_cell, wall_name):
            self.robot.crashed = True
            return {
                "ok": False,
                "error": "wall_collision",
                "x": self.robot.x,
                "y": self.robot.y,
                "status": self.status(),
            }

        new_x = self.robot.x + move_info["dx"]
        new_y = self.robot.y + move_info["dy"]

        if not self.maze.in_bounds(new_x, new_y):
            self.robot.crashed = True
            return {
                "ok": False,
                "error": "out_of_bounds",
                "x": self.robot.x,
                "y": self.robot.y,
                "status": self.status(),
            }

        if self.maze.get_cell(new_x, new_y).blocked:
            self.robot.crashed = True
            return {
                "ok": False,
                "error": "blocked_cell",
                "x": self.robot.x,
                "y": self.robot.y,
                "status": self.status(),
            }

        self.robot.x = new_x
        self.robot.y = new_y

        if self.robot.x == self.maze.exit_x and self.robot.y == self.maze.exit_y:
            self.robot.finished = True

        return {
            "ok": True,
            "x": self.robot.x,
            "y": self.robot.y,
            "status": self.status(),
        }

    def reset(self):
        self.robot.reset()

    def to_json(self):
        return {
            "width": self.maze.width,
            "height": self.maze.height,
            "robot": {
                "x": self.robot.x,
                "y": self.robot.y,
            },
            "exit": {
                "x": self.maze.exit_x,
                "y": self.maze.exit_y,
            },
            "status": self.status(),
            "cells": [
                [self.maze.cells[y][x].to_json() for x in range(self.maze.width)]
                for y in range(self.maze.height)
            ],
        }

    def xray(self):
        result = []
        radius = 2

        for dy in range(-radius, radius + 1):
            row = []

            for dx in range(-radius, radius + 1):
                x = self.robot.x + dx
                y = self.robot.y + dy

                if self.maze.in_bounds(x, y):
                    cell = self.maze.get_cell(x, y)

                    cell_info = {
                        "outside": False,
                        "top": cell.top,
                        "right": cell.right,
                        "down": cell.down,
                        "left": cell.left,
                    }

                    row.append(cell_info)
                else:
                    cell_info = {
                        "outside": True,
                        "top": True,
                        "right": True,
                        "down": True,
                        "left": True,
                    }

                    row.append(cell_info)

            result.append(row)

        return {
            "width": 5,
            "height": 5,
            "robot_index": {
                "x": 2,
                "y": 2,
            },
            "cells": result,
        }