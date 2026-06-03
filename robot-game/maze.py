from cell import Cell


class Maze:
    def __init__(self, width: int, height: int):
        self.width = width
        self.height = height
        self.cells = [[Cell() for _ in range(width)] for _ in range(height)]
        self.exit_x = 0
        self.exit_y = 0
        self.add_outer_walls()

    def add_outer_walls(self):
        for y in range(self.height):
            for x in range(self.width):
                if y == 0:
                    self.cells[y][x].top = True
                if y == self.height - 1:
                    self.cells[y][x].down = True
                if x == 0:
                    self.cells[y][x].left = True
                if x == self.width - 1:
                    self.cells[y][x].right = True

    def in_bounds(self, x: int, y: int) -> bool:
        return 0 <= x < self.width and 0 <= y < self.height

    def get_cell(self, x: int, y: int) -> Cell:
        return self.cells[y][x]

    def build_walls_from_blocked_cells(self):
        self.add_outer_walls()

        for y in range(self.height):
            for x in range(self.width):
                if not self.cells[y][x].blocked:
                    continue

                blocked_cell = self.cells[y][x]
                blocked_cell.top = True
                blocked_cell.right = True
                blocked_cell.down = True
                blocked_cell.left = True

                if self.in_bounds(x, y - 1):
                    self.cells[y - 1][x].down = True

                if self.in_bounds(x + 1, y):
                    self.cells[y][x + 1].left = True

                if self.in_bounds(x, y + 1):
                    self.cells[y + 1][x].top = True

                if self.in_bounds(x - 1, y):
                    self.cells[y][x - 1].right = True

    @staticmethod
    def load_from_file(path: str):
        with open(path, "r", encoding="utf-8") as file:
            lines = [line.strip() for line in file if line.strip()]

        width, height = map(int, lines[0].split())
        robot_x, robot_y = map(int, lines[1].split())
        exit_x, exit_y = map(int, lines[2].split())

        grid = lines[3:3 + height]

        if len(grid) != height:
            raise ValueError(f"Invalid maze height")

        for row in grid:
            if len(row) != width:
                raise ValueError(f"Invalid maze width")

        maze = Maze(width, height)
        maze.exit_x = exit_x
        maze.exit_y = exit_y

        for y in range(height):
            for x in range(width):
                symbol = grid[y][x]

                if symbol == "#":
                    maze.cells[y][x].blocked = True
                elif symbol == ".":
                    pass
                else:
                    raise ValueError(f"Unknown maze symbol")

        maze.build_walls_from_blocked_cells()

        if not maze.in_bounds(robot_x, robot_y):
            raise ValueError("Robot start position is out of bounds")

        if not maze.in_bounds(exit_x, exit_y):
            raise ValueError("Exit position is out of bounds")

        if maze.get_cell(robot_x, robot_y).blocked:
            raise ValueError("Robot start position is blocked")

        if maze.get_cell(exit_x, exit_y).blocked:
            raise ValueError("Exit position is blocked")

        return maze, robot_x, robot_y