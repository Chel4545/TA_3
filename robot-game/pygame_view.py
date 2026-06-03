import pygame

from cell import Cell


class PygameView:
    def __init__(self, state, state_lock):
        pygame.init()

        self.state = state
        self.state_lock = state_lock

        self.cell_size = 80
        self.margin = 40
        self.info_height = 80

        width_px = self.margin * 2 + self.state.maze.width * self.cell_size
        height_px = self.margin * 2 + self.state.maze.height * self.cell_size + self.info_height

        self.screen = pygame.display.set_mode((width_px, height_px))
        pygame.display.set_caption("Robot Maze")

        self.clock = pygame.time.Clock()
        self.font = pygame.font.SysFont("Arial", 22)

    def draw(self):
        self.screen.fill((245, 245, 245))

        with self.state_lock:
            maze = self.state.maze
            robot = self.state.robot
            status = self.state.status()

            for y in range(maze.height):
                for x in range(maze.width):
                    self.draw_cell(x, y, maze.get_cell(x, y))

            self.draw_exit(maze.exit_x, maze.exit_y)
            self.draw_robot(robot.x, robot.y, status)
            self.draw_info(status)

        pygame.display.flip()

    def draw_cell(self, x: int, y: int, cell: Cell):
        left = self.margin + x * self.cell_size
        top = self.margin + y * self.cell_size
        right = left + self.cell_size
        bottom = top + self.cell_size

        if cell.blocked:
            pygame.draw.rect(
                self.screen,
                (40, 40, 40),
                (left, top, self.cell_size, self.cell_size),
            )
            return

        pygame.draw.rect(
            self.screen,
            (255, 255, 255),
            (left, top, self.cell_size, self.cell_size),
        )

        pygame.draw.rect(
            self.screen,
            (220, 220, 220),
            (left, top, self.cell_size, self.cell_size),
            1,
        )

        wall_color = (0, 0, 0)
        wall_width = 5

        if cell.top:
            pygame.draw.line(
                self.screen,
                wall_color,
                (left, top),
                (right, top),
                wall_width,
            )

        if cell.right:
            pygame.draw.line(
                self.screen,
                wall_color,
                (right, top),
                (right, bottom),
                wall_width,
            )

        if cell.down:
            pygame.draw.line(
                self.screen,
                wall_color,
                (left, bottom),
                (right, bottom),
                wall_width,
            )

        if cell.left:
            pygame.draw.line(
                self.screen,
                wall_color,
                (left, top),
                (left, bottom),
                wall_width,
            )

    def draw_robot(self, x: int, y: int, status: str):
        center_x = self.margin + x * self.cell_size + self.cell_size // 2
        center_y = self.margin + y * self.cell_size + self.cell_size // 2

        if status == "crashed":
            color = (200, 40, 40)
        elif status == "finished":
            color = (40, 170, 70)
        else:
            color = (50, 100, 220)

        pygame.draw.circle(
            self.screen,
            color,
            (center_x, center_y),
            self.cell_size // 4,
        )

    def draw_exit(self, x: int, y: int):
        left = self.margin + x * self.cell_size + 20
        top = self.margin + y * self.cell_size + 20
        size = self.cell_size - 40

        pygame.draw.rect(
            self.screen,
            (80, 200, 80),
            (left, top, size, size),
        )

    def draw_info(self, status: str):
        if status == "crashed":
            text = "Robot crashed! Press R to reset."
        elif status == "finished":
            text = "Exit reached! Press R to reset."
        else:
            text = "Arrows/WASD: move | R: reset | API: http://127.0.0.1:5000"

        label = self.font.render(text, True, (30, 30, 30))

        info_y = self.margin + self.state.maze.height * self.cell_size + 25
        self.screen.blit(label, (self.margin, info_y))

    def handle_key(self, key):
        direction = None

        if key in (pygame.K_UP, pygame.K_w):
            direction = "top"
        elif key in (pygame.K_DOWN, pygame.K_s):
            direction = "bottom"
        elif key in (pygame.K_LEFT, pygame.K_a):
            direction = "left"
        elif key in (pygame.K_RIGHT, pygame.K_d):
            direction = "right"
        elif key == pygame.K_r:
            with self.state_lock:
                self.state.reset()
            return

        if direction is not None:
            with self.state_lock:
                self.state.move(direction)

    def run(self):
        running = True

        while running:
            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    running = False

                elif event.type == pygame.KEYDOWN:
                    self.handle_key(event.key)

            self.draw()
            self.clock.tick(60)

        pygame.quit()