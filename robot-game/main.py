import os
from threading import Thread, Lock

from maze import Maze
from robot import Robot
from game_state import GameState
from api import create_app, run_api_server
from pygame_view import PygameView


def main():
    base_dir = os.path.dirname(os.path.abspath(__file__))
    maze_path = os.path.join(base_dir, "mazes", "maze1.txt")

    maze, robot_x, robot_y = Maze.load_from_file(maze_path)
    robot = Robot(robot_x, robot_y)
    game_state = GameState(maze, robot)

    state_lock = Lock()

    app = create_app(game_state, state_lock)

    api_thread = Thread(
        target=run_api_server,
        args=(app,),
        daemon=True,
    )
    api_thread.start()

    view = PygameView(game_state, state_lock)
    view.run()


if __name__ == "__main__":
    main()