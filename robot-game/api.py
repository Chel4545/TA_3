from flask import Flask, jsonify


def create_app(game_state, state_lock):
    app = Flask(__name__)

    @app.get("/state")
    def get_state():
        with state_lock:
            return jsonify(game_state.to_json())

    @app.get("/xray")
    def get_xray():
        with state_lock:
            return jsonify(game_state.xray())

    @app.post("/reset")
    def reset():
        with state_lock:
            game_state.reset()
            return jsonify({
                "ok": True,
                "status": game_state.status(),
            })

    @app.post("/move/<direction>")
    def move(direction):
        with state_lock:
            result = game_state.move(direction)
            return jsonify(result)

    return app


def run_api_server(app):
    app.run(
        host="127.0.0.1",
        port=5000,
        debug=False,
        use_reloader=False,
    )