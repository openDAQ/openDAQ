"""Example openDAQ Python plugin: playable duck games in real windows - purely for fun.

Includes:
- FlappyDuckFb: flappy-bird style game (any key to flap)
- StarWarsDuckFb: Star-Wars-style space shooter (move with arrows/WASD, space to shoot)

Same live-window architecture as plotter_module.py (see that file's module docstring for the
main-thread/GIL constraints). FlappyDuckFb wires a nested KeyboardFb for openDAQ signal flow;
each keypress is published as a Float64 code (ord() for letters, dedicated codes for arrows etc.).
StarWarsDuckFb reads held keys directly from the matplotlib window (movement needs continuous
input, not discrete pulses).

Close the game window (or Ctrl+C) to quit.

Runnable directly as `python flappy_duck_module.py [flappy|starwars]` - or embedded from a C++
host, see plotter_module.py's own docstring for both ways to load a plugin file.

Requires numpy (not an openDAQ dependency - install separately: `pip install numpy`).
"""

import contextlib
import io
import os
import random
import shutil
import subprocess
import sys
import tempfile
import threading
import wave
from time import sleep

import matplotlib.pyplot as plt
import numpy as np
from matplotlib.patches import Circle, Ellipse, Polygon, Rectangle

import opendaq

_WIDTH = 40.0
_HEIGHT = 18.0
_GRAVITY = 0.35
_FLAP_VELOCITY = 1.6
_PIPE_WIDTH = 3.0
_PIPE_GAP = 6.0
_PIPE_SPACING = 14.0
_PIPE_SPEED = 0.6
_DUCK_X = 8.0
_DUCK_HALF_WIDTH = 1.1
_TICK_S = 0.05

# Star Wars duck shooter
_SW_WIDTH = 36.0
_SW_HEIGHT = 24.0
_SW_TICK_S = 0.03
_SW_DUCK_SPEED = 1.35
_SW_LASER_SPEED = 3.4
_SW_ENEMY_SPEED = 1.0
_SW_ENEMY_LASER_SPEED = 2.8
_SW_STAR_SPEED = 0.4
_SW_SHOOT_COOLDOWN_TICKS = 2
_SW_ENEMY_SPAWN_INTERVAL = 22
_SW_ENEMY_SPAWN_COUNT_MIN = 1
_SW_ENEMY_SPAWN_COUNT_MAX = 3
_SW_MAX_ENEMIES = 10
_SW_ENEMY_SHOOT_FIRST_MIN = 4
_SW_ENEMY_SHOOT_FIRST_MAX = 14
_SW_ENEMY_SHOOT_INTERVAL_MIN = 12
_SW_ENEMY_SHOOT_INTERVAL_MAX = 28
_SW_DUCK_HALF_W = 1.9
_SW_DUCK_HALF_H = 1.3
_SW_LASER_HALF_W = 0.15
_SW_LASER_HALF_H = 0.7
_SW_ENEMY_HALF_W = 1.4
_SW_ENEMY_HALF_H = 0.6


def _make_beep_wav(freq: float, duration: float, volume: float = 0.25) -> bytes:
    sample_rate = 22050
    sample_count = max(1, int(sample_rate * duration))
    t = np.linspace(0, duration, sample_count, endpoint=False)
    attack = min(0.008, duration * 0.25)
    release = min(0.015, duration * 0.35)
    envelope = np.ones(sample_count, dtype=np.float64)
    attack_samples = max(1, int(attack * sample_rate))
    release_samples = max(1, int(release * sample_rate))
    envelope[:attack_samples] = np.linspace(0, 1, attack_samples)
    envelope[-release_samples:] = np.linspace(1, 0, release_samples)
    samples = (volume * 32767 * envelope * np.sin(2 * np.pi * freq * t)).astype(np.int16)
    buf = io.BytesIO()
    with wave.open(buf, "wb") as wf:
        wf.setnchannels(1)
        wf.setsampwidth(2)
        wf.setframerate(sample_rate)
        wf.writeframes(samples.tobytes())
    return buf.getvalue()


def _play_wav_async(wav_bytes: bytes) -> None:
    def _play() -> None:
        path = None
        try:
            with tempfile.NamedTemporaryFile(suffix=".wav", delete=False) as tmp:
                tmp.write(wav_bytes)
                path = tmp.name
            if sys.platform == "darwin":
                subprocess.Popen(
                    ["afplay", "-v", "0.35", path],
                    stdout=subprocess.DEVNULL,
                    stderr=subprocess.DEVNULL,
                )
            elif sys.platform == "win32":
                import winsound

                winsound.PlaySound(path, winsound.SND_FILENAME | winsound.SND_ASYNC)
            elif shutil.which("aplay"):
                subprocess.Popen(
                    ["aplay", "-q", path],
                    stdout=subprocess.DEVNULL,
                    stderr=subprocess.DEVNULL,
                )
            else:
                return
            sleep(0.12)
        except OSError:
            return
        finally:
            if path is not None:
                with contextlib.suppress(OSError):
                    os.unlink(path)

    threading.Thread(target=_play, daemon=True).start()


_PLAYER_SHOOT_SOUND = _make_beep_wav(1500, 0.045, 0.22)
_ENEMY_SHOOT_SOUND = _make_beep_wav(650, 0.075, 0.16)


def _play_player_shoot_sound() -> None:
    _play_wav_async(_PLAYER_SHOOT_SOUND)


def _play_enemy_shoot_sound() -> None:
    _play_wav_async(_ENEMY_SHOOT_SOUND)


def _disable_macos_ctrl_c_interrupt_if_not_pythons_main_thread() -> None:
    """Same workaround as plotter_module.py - see that file for the full explanation. Only
    bites when this is loaded into a C++ host that embeds Python (PythonRuntime owns a
    different "main thread" than the one show() actually runs on); a no-op on the real
    load_python_module path this file's own main() uses."""
    if threading.current_thread() is threading.main_thread():
        return

    try:
        import matplotlib.backends.backend_macosx as backend_macosx
    except ImportError:
        return

    backend_macosx._allow_interrupt_macos = contextlib.nullcontext


def _draw_duck(ax, x: float, y: float) -> None:
    ax.add_patch(Ellipse((x, y), width=2.2, height=1.4, facecolor="#f5c518", edgecolor="#8a6a08", zorder=3))
    ax.add_patch(Circle((x + 0.7, y + 0.6), radius=0.55, facecolor="#f5c518", edgecolor="#8a6a08", zorder=3))
    ax.add_patch(
        Polygon(
            [(x + 1.15, y + 0.55), (x + 1.8, y + 0.45), (x + 1.15, y + 0.35)],
            closed=True,
            facecolor="#e8720c",
            edgecolor="#8a6a08",
            zorder=4,
        )
    )
    ax.add_patch(Circle((x + 0.85, y + 0.75), radius=0.08, facecolor="black", zorder=4))


def _draw_duck_ship(ax, x: float, y: float) -> None:
    """Duck-as-X-Wing: body pointing up, wings spread."""
    ax.add_patch(Ellipse((x, y), width=1.4, height=2.2, facecolor="#f5c518", edgecolor="#8a6a08", zorder=4))
    ax.add_patch(
        Polygon(
            [(x - 2.0, y - 0.3), (x - 0.6, y + 0.2), (x - 0.6, y - 0.8)],
            closed=True,
            facecolor="#d4a90f",
            edgecolor="#8a6a08",
            zorder=3,
        )
    )
    ax.add_patch(
        Polygon(
            [(x + 2.0, y - 0.3), (x + 0.6, y + 0.2), (x + 0.6, y - 0.8)],
            closed=True,
            facecolor="#d4a90f",
            edgecolor="#8a6a08",
            zorder=3,
        )
    )
    ax.add_patch(
        Polygon(
            [(x - 0.15, y + 0.9), (x + 0.15, y + 0.9), (x, y + 1.5)],
            closed=True,
            facecolor="#e8720c",
            edgecolor="#8a6a08",
            zorder=5,
        )
    )
    ax.add_patch(Circle((x, y + 0.4), radius=0.18, facecolor="black", zorder=5))


def _draw_tie_fighter(ax, x: float, y: float) -> None:
    ax.add_patch(Circle((x, y), radius=0.55, facecolor="#2a2a2a", edgecolor="#666666", zorder=3))
    ax.add_patch(Rectangle((x - 1.4, y - 0.15), 0.35, 0.3, facecolor="#555555", edgecolor="#888888", zorder=2))
    ax.add_patch(Rectangle((x + 1.05, y - 0.15), 0.35, 0.3, facecolor="#555555", edgecolor="#888888", zorder=2))
    ax.add_patch(
        Polygon(
            [(x - 0.25, y), (x + 0.25, y), (x, y - 0.5)],
            closed=True,
            facecolor="#444444",
            edgecolor="#777777",
            zorder=4,
        )
    )


def _draw_laser(ax, x: float, y: float) -> None:
    ax.add_patch(Rectangle((x - 0.08, y - 0.6), 0.16, 1.2, facecolor="#ffee55", edgecolor="#ccaa00", zorder=5))


def _draw_enemy_laser(ax, x: float, y: float) -> None:
    ax.add_patch(Rectangle((x - 0.08, y - 0.6), 0.16, 1.2, facecolor="#ff5533", edgecolor="#aa2200", zorder=5))


def _draw_pipe(ax, x: float, gap_bottom: float) -> None:
    ax.add_patch(Rectangle((x, 0), _PIPE_WIDTH, gap_bottom, facecolor="#4caf50", edgecolor="#2e7031"))
    ax.add_patch(
        Rectangle(
            (x, gap_bottom + _PIPE_GAP),
            _PIPE_WIDTH,
            _HEIGHT - (gap_bottom + _PIPE_GAP),
            facecolor="#4caf50",
            edgecolor="#2e7031",
        )
    )


class _Pipe:
    __slots__ = ("x", "gap_bottom", "scored")

    def __init__(self, x: float, gap_bottom: float):
        self.x = x
        self.gap_bottom = gap_bottom
        self.scored = False


class _Laser:
    __slots__ = ("x", "y", "dy")

    def __init__(self, x: float, y: float, dy: float):
        self.x = x
        self.y = y
        self.dy = dy


class _Enemy:
    __slots__ = ("x", "y", "shoot_timer")

    def __init__(self, x: float, y: float):
        self.x = x
        self.y = y
        self.shoot_timer = random.randint(_SW_ENEMY_SHOOT_FIRST_MIN, _SW_ENEMY_SHOOT_FIRST_MAX)


def _ranges_overlap(min_a: float, max_a: float, min_b: float, max_b: float) -> bool:
    return max_a >= min_b and min_a <= max_b


def _player_laser_hits_enemy(laser_x: float, y_min: float, y_max: float, enemy: _Enemy) -> bool:
    if abs(laser_x - enemy.x) > _SW_LASER_HALF_W + _SW_ENEMY_HALF_W:
        return False
    return _ranges_overlap(y_min, y_max, enemy.y - _SW_ENEMY_HALF_H, enemy.y + _SW_ENEMY_HALF_H)


def _enemy_laser_hits_duck(
    laser_x: float,
    y_min: float,
    y_max: float,
    duck_x_min: float,
    duck_x_max: float,
    duck_y_min: float,
    duck_y_max: float,
) -> bool:
    laser_x_min = laser_x - _SW_LASER_HALF_W
    laser_x_max = laser_x + _SW_LASER_HALF_W
    return _ranges_overlap(laser_x_min, laser_x_max, duck_x_min, duck_x_max) and _ranges_overlap(
        y_min, y_max, duck_y_min, duck_y_max
    )


def _duck_hits_enemy(
    duck_x_min: float,
    duck_x_max: float,
    duck_y_min: float,
    duck_y_max: float,
    enemy: _Enemy,
) -> bool:
    enemy_x_min = enemy.x - _SW_ENEMY_HALF_W
    enemy_x_max = enemy.x + _SW_ENEMY_HALF_W
    enemy_y_min = enemy.y - _SW_ENEMY_HALF_H
    enemy_y_max = enemy.y + _SW_ENEMY_HALF_H
    return _ranges_overlap(duck_x_min, duck_x_max, enemy_x_min, enemy_x_max) and _ranges_overlap(
        duck_y_min, duck_y_max, enemy_y_min, enemy_y_max
    )


class _Star:
    __slots__ = ("x", "y", "speed")

    def __init__(self, x: float, y: float, speed: float):
        self.x = x
        self.y = y
        self.speed = speed


def _normalize_key(key: str | None) -> str | None:
    if key is None:
        return None
    return key.lower()


def _movement_delta(keys: set[str], speed: float) -> tuple[float, float]:
    dx = dy = 0.0
    if "left" in keys or "a" in keys:
        dx -= speed
    if "right" in keys or "d" in keys:
        dx += speed
    if "up" in keys or "w" in keys:
        dy += speed
    if "down" in keys or "s" in keys:
        dy -= speed
    return dx, dy


def _shoot_requested(keys: set[str]) -> bool:
    return " " in keys or "space" in keys


_SPECIAL_KEY_CODES = {
    "left": 0xF000,
    "right": 0xF001,
    "up": 0xF002,
    "down": 0xF003,
    "space": 0xF004,
    "escape": 0xF005,
    "enter": 0xF006,
    "shift": 0xF007,
    "control": 0xF008,
    "alt": 0xF009,
    "tab": 0xF00A,
}
_CODE_TO_KEY = {code: name for name, code in _SPECIAL_KEY_CODES.items()}


def _key_to_sample(key: str) -> float:
    normalized = key.lower()
    if normalized in _SPECIAL_KEY_CODES:
        return float(_SPECIAL_KEY_CODES[normalized])
    if len(normalized) == 1:
        return float(ord(normalized))
    return float(ord(normalized[0]))


def _sample_to_key(code: float) -> str:
    value = int(code)
    if value in _CODE_TO_KEY:
        return _CODE_TO_KEY[value]
    return chr(value)


class KeyboardFb(opendaq.FunctionBlock):
    """No input port - exposes an output signal on every keypress. Each sample is a Float64 key
    code: ord() for printable keys ('a' -> 97), dedicated codes for arrows/space/etc. (see
    _SPECIAL_KEY_CODES). Decode with _sample_to_key().
    """

    @staticmethod
    def create_function_block_type() -> opendaq.IFunctionBlockType:
        return opendaq.FunctionBlockType(
            "KeyboardFb",
            "Keyboard",
            "Publishes a signal with the pressed key code on every keypress",
            None,
        )

    def on_init(self) -> None:
        self.domain_signal = opendaq.Signal(self.context, self.signals_folder, "domain", None)
        domain_desc = opendaq.DataDescriptorBuilder()
        domain_desc.sample_type = opendaq.SampleType.Int64
        domain_desc.tick_resolution = opendaq.Ratio(1, 1000)
        domain_desc.rule = opendaq.LinearDataRule(1, 0)
        domain_desc.origin = "1970-01-01T00:00:00"
        self._domain_descriptor = domain_desc.build()
        self.domain_signal.descriptor = self._domain_descriptor

        self.keypress_signal = opendaq.Signal(self.context, self.signals_folder, "keypress", None)
        value_desc = opendaq.DataDescriptorBuilder()
        value_desc.sample_type = opendaq.SampleType.Float64
        self._value_descriptor = value_desc.build()
        self.keypress_signal.descriptor = self._value_descriptor
        self.keypress_signal.domain_signal = self.domain_signal
        self.add_signal(self.keypress_signal)

        self._domain_tick = 0

    def emit_keypress(self, key: str) -> None:
        domain_packet = opendaq.DataPacket(self._domain_descriptor, 1, self._domain_tick)
        value_packet = opendaq.DataPacketWithDomain(domain_packet, self._value_descriptor, 1, 0)
        np.frombuffer(value_packet.raw_data, dtype=np.float64)[0] = _key_to_sample(key)
        self.domain_signal.send_packet(domain_packet)
        self.keypress_signal.send_packet(value_packet)
        self._domain_tick += 1


class FlappyDuckFb(opendaq.FunctionBlock):
    """Creates a nested KeyboardFb on init and connects it to its one input port - every sample
    that arrives, regardless of value, flaps the duck once; it falls under gravity otherwise.
    Close the window or Ctrl+C to quit.
    """

    def __init__(self, context: opendaq.IContext, parent: opendaq.IComponent, local_id: str) -> None:
        super().__init__(context, parent, local_id)
        # Figure/Axes are created lazily in show(), not here - see plotter_module.py's __init__
        # for why (must happen on the main thread, which __init__ isn't guaranteed to be).
        self.figure = None
        self.axes = None
        self._pending_flaps = 0
        self._keyboard_delegate: KeyboardFb | None = None
        self._reset()

    def _reset(self) -> None:
        self.duck_y = _HEIGHT / 2
        self.velocity = 0.0
        self.pipes = [_Pipe(_WIDTH, self._random_gap_bottom())]
        self.score = 0
        self.game_over = False

    @staticmethod
    def _random_gap_bottom() -> float:
        return random.uniform(1.0, _HEIGHT - _PIPE_GAP - 1.0)

    @staticmethod
    def create_function_block_type() -> opendaq.IFunctionBlockType:
        return opendaq.FunctionBlockType(
            "FlappyDuckFb",
            "Flappy Duck",
            "A playable window game - press any key to flap, close the window to quit",
            None,
        )

    def _create_nested_keyboard(self) -> opendaq.IFunctionBlock:
        fb_folder = self._cpp_fb.ref.get_item("FB")
        for module in self.context.module_manager.modules:
            if module.module_info.id != "FlappyDuckModuleId":
                continue
            keyboard_fb = module.create_function_block("KeyboardFb", fb_folder, "keyboard", None)
            self.add_nested_function_block(keyboard_fb)
            self._keyboard_delegate = FlappyDuckModule.keyboard_delegate("keyboard")
            return keyboard_fb
        raise RuntimeError("FlappyDuckModule is not loaded")

    def _on_key_press(self, event) -> None:
        if event.key is None or self._keyboard_delegate is None:
            return
        self._keyboard_delegate.emit_keypress(event.key)

    def on_init(self) -> None:
        self._keypress_port = opendaq.InputPort(self.context, self.input_ports_folder, "keypress", False)
        self.add_input_port(self._keypress_port)
        # Nested FB creation must not run synchronously here: on_init() is reached from
        # PythonRuntime's dispatch thread while add_function_block() is still blocked waiting
        # on it, and module.create_function_block() would need that same thread again.
        self.context.scheduler.schedule_work_on_main_loop(opendaq.Work(self._init_nested_keyboard))

    def on_packet_received(self, port: opendaq.IInputPort) -> None:
        connection = port.connection
        self._pending_flaps += connection.available_samples
        connection.dequeue_all()

    def _init_nested_keyboard(self) -> None:
        fb = self._cpp_fb
        if fb.ref is None:
            return

        keyboard_fb = self._create_nested_keyboard()
        self._keypress_port.connect(keyboard_fb.signals[0])  # only keypress is published
        self.context.scheduler.schedule_work_on_main_loop(opendaq.Work(self.show))

    def _flap_requested(self) -> bool:
        # on_packet_received() is async (dispatched off the main loop thread), so also drain
        # the port queue here on the same thread as show() to avoid missing same-frame keypresses.
        connection = self._keypress_port.connection
        if connection is not None and connection.available_samples > 0:
            self._pending_flaps += connection.available_samples
            connection.dequeue_all()
        if self._pending_flaps == 0:
            return False
        self._pending_flaps -= 1
        return True

    def show(self, interval_s: float = _TICK_S) -> None:
        fb = self._cpp_fb
        if fb.ref is None:
            return  # backing function block already destroyed - stop rescheduling

        if self.figure is None:
            _disable_macos_ctrl_c_interrupt_if_not_pythons_main_thread()
            self.figure, self.axes = plt.subplots()
            self.figure.canvas.manager.set_window_title("Flappy Duck")
            self.figure.canvas.mpl_connect("key_press_event", self._on_key_press)
            plt.show(block=False)

        if not plt.fignum_exists(self.figure.number):
            return  # window closed - nothing more to draw, and nothing to reschedule

        if self._flap_requested():
            if self.game_over:
                self._reset()
            else:
                self.velocity = _FLAP_VELOCITY

        if not self.game_over:
            self._advance()

        self._draw()
        plt.pause(interval_s)

        self.context.scheduler.schedule_work_on_main_loop(opendaq.Work(self.show))

    def _advance(self) -> None:
        self.velocity -= _GRAVITY
        self.duck_y += self.velocity

        for pipe in self.pipes:
            pipe.x -= _PIPE_SPEED
        if self.pipes and self.pipes[0].x + _PIPE_WIDTH < 0:
            self.pipes.pop(0)
        if self.pipes[-1].x < _WIDTH - _PIPE_SPACING:
            self.pipes.append(_Pipe(_WIDTH, self._random_gap_bottom()))

        if self.duck_y < 0 or self.duck_y > _HEIGHT:
            self.game_over = True
            return

        duck_left, duck_right = _DUCK_X - _DUCK_HALF_WIDTH, _DUCK_X + _DUCK_HALF_WIDTH
        for pipe in self.pipes:
            if pipe.x < duck_right and duck_left < pipe.x + _PIPE_WIDTH:
                if not (pipe.gap_bottom < self.duck_y < pipe.gap_bottom + _PIPE_GAP):
                    self.game_over = True
            if not pipe.scored and pipe.x + _PIPE_WIDTH < duck_left:
                pipe.scored = True
                self.score += 1

    def _draw(self) -> None:
        ax = self.axes
        ax.cla()
        ax.set_xlim(0, _WIDTH)
        ax.set_ylim(0, _HEIGHT)
        ax.set_xticks([])
        ax.set_yticks([])
        ax.set_facecolor("#bfe6ff")

        for pipe in self.pipes:
            _draw_pipe(ax, pipe.x, pipe.gap_bottom)
        _draw_duck(ax, _DUCK_X, self.duck_y)

        ax.text(0.5, _HEIGHT - 1, f"score: {self.score}", fontsize=12)
        if self.game_over:
            ax.text(
                _WIDTH / 2,
                _HEIGHT / 2,
                "GAME OVER\npress any key to try again",
                ha="center",
                va="center",
                fontsize=14,
                color="#b00020",
                weight="bold",
            )


class StarWarsDuckFb(opendaq.FunctionBlock):
    """Top-down space shooter: fly the duck, blast TIE fighters. Arrow keys / WASD to move,
    space to shoot. Keys are read directly from the matplotlib window (held keys for movement).
    """

    def __init__(self, context: opendaq.IContext, parent: opendaq.IComponent, local_id: str) -> None:
        super().__init__(context, parent, local_id)
        self.figure = None
        self.axes = None
        self._keys_down: set[str] = set()
        self._reset()

    @staticmethod
    def create_function_block_type() -> opendaq.IFunctionBlockType:
        return opendaq.FunctionBlockType(
            "StarWarsDuckFb",
            "Star Wars Duck",
            "Space shooter - arrows/WASD to move, space to shoot, close window to quit",
            None,
        )

    def _reset(self) -> None:
        self.duck_x = _SW_WIDTH / 2
        self.duck_y = 3.0
        self.lasers: list[_Laser] = []
        self.enemy_lasers: list[_Laser] = []
        self.enemies: list[_Enemy] = []
        self.stars = [
            _Star(random.uniform(0, _SW_WIDTH), random.uniform(0, _SW_HEIGHT), random.uniform(0.15, _SW_STAR_SPEED))
            for _ in range(60)
        ]
        self.score = 0
        self.game_over = False
        self._shoot_cooldown = 0
        self._enemy_spawn_timer = _SW_ENEMY_SPAWN_INTERVAL

    def _spawn_enemies(self) -> None:
        slots = _SW_MAX_ENEMIES - len(self.enemies)
        if slots <= 0:
            return
        count = min(random.randint(_SW_ENEMY_SPAWN_COUNT_MIN, _SW_ENEMY_SPAWN_COUNT_MAX), slots)
        for _ in range(count):
            x = random.uniform(1.5, _SW_WIDTH - 1.5)
            y = _SW_HEIGHT + 1 + random.uniform(0, 4)
            self.enemies.append(_Enemy(x, y))

    def _enemy_on_screen(self, enemy: _Enemy) -> bool:
        return 1.5 < enemy.y < _SW_HEIGHT - 1.0

    def _enemy_shoots(self, enemy: _Enemy) -> None:
        self.enemy_lasers.append(_Laser(enemy.x, enemy.y - 0.8, -_SW_ENEMY_LASER_SPEED))
        enemy.shoot_timer = random.randint(_SW_ENEMY_SHOOT_INTERVAL_MIN, _SW_ENEMY_SHOOT_INTERVAL_MAX)
        _play_enemy_shoot_sound()

    def on_init(self) -> None:
        self.context.scheduler.schedule_work_on_main_loop(opendaq.Work(self.show))

    def _on_key_press(self, event) -> None:
        key = _normalize_key(event.key)
        if key is None:
            return
        self._keys_down.add(key)
        if self.game_over and key not in ("shift", "control", "alt", "meta"):
            self._reset()

    def _on_key_release(self, event) -> None:
        key = _normalize_key(event.key)
        if key is not None:
            self._keys_down.discard(key)

    def show(self, interval_s: float = _SW_TICK_S) -> None:
        fb = self._cpp_fb
        if fb.ref is None:
            return

        if self.figure is None:
            _disable_macos_ctrl_c_interrupt_if_not_pythons_main_thread()
            self.figure, self.axes = plt.subplots()
            self.figure.canvas.manager.set_window_title("Star Wars Duck")
            self.figure.canvas.mpl_connect("key_press_event", self._on_key_press)
            self.figure.canvas.mpl_connect("key_release_event", self._on_key_release)
            plt.show(block=False)

        if not plt.fignum_exists(self.figure.number):
            return

        if not self.game_over:
            self._advance()

        self._draw()
        plt.pause(interval_s)
        self.context.scheduler.schedule_work_on_main_loop(opendaq.Work(self.show))

    def _advance(self) -> None:
        old_duck_x = self.duck_x
        old_duck_y = self.duck_y
        dx, dy = _movement_delta(self._keys_down, _SW_DUCK_SPEED)
        self.duck_x = max(1.0, min(_SW_WIDTH - 1.0, self.duck_x + dx))
        self.duck_y = max(1.0, min(_SW_HEIGHT - 1.0, self.duck_y + dy))
        duck_x_min = min(old_duck_x, self.duck_x) - _SW_DUCK_HALF_W
        duck_x_max = max(old_duck_x, self.duck_x) + _SW_DUCK_HALF_W
        duck_y_min = min(old_duck_y, self.duck_y) - _SW_DUCK_HALF_H
        duck_y_max = max(old_duck_y, self.duck_y) + _SW_DUCK_HALF_H

        if self._shoot_cooldown > 0:
            self._shoot_cooldown -= 1
        elif _shoot_requested(self._keys_down):
            self.lasers.append(_Laser(self.duck_x, self.duck_y + 1.2, _SW_LASER_SPEED))
            self._shoot_cooldown = _SW_SHOOT_COOLDOWN_TICKS
            _play_player_shoot_sound()

        self._enemy_spawn_timer -= 1
        if self._enemy_spawn_timer <= 0:
            self._spawn_enemies()
            self._enemy_spawn_timer = random.randint(_SW_ENEMY_SPAWN_INTERVAL - 6, _SW_ENEMY_SPAWN_INTERVAL + 8)

        for enemy in self.enemies:
            enemy.y -= _SW_ENEMY_SPEED
            if not self._enemy_on_screen(enemy):
                continue
            enemy.shoot_timer -= 1
            if enemy.shoot_timer <= 0:
                self._enemy_shoots(enemy)
        self.enemies = [enemy for enemy in self.enemies if enemy.y > -1]

        hit_enemies: set[int] = set()
        lasers_next: list[_Laser] = []
        for laser in self.lasers:
            old_y = laser.y
            laser.y += laser.dy
            if laser.y >= _SW_HEIGHT + 1:
                continue
            y_min = min(old_y, laser.y) - _SW_LASER_HALF_H
            y_max = max(old_y, laser.y) + _SW_LASER_HALF_H
            destroyed = False
            for i, enemy in enumerate(self.enemies):
                if i in hit_enemies:
                    continue
                if _player_laser_hits_enemy(laser.x, y_min, y_max, enemy):
                    hit_enemies.add(i)
                    self.score += 1
                    destroyed = True
                    break
            if not destroyed:
                lasers_next.append(laser)
        self.lasers = lasers_next
        self.enemies = [enemy for i, enemy in enumerate(self.enemies) if i not in hit_enemies]

        enemy_lasers_next: list[_Laser] = []
        for blast in self.enemy_lasers:
            old_y = blast.y
            blast.y += blast.dy
            if blast.y < -1:
                continue
            y_min = min(old_y, blast.y) - _SW_LASER_HALF_H
            y_max = max(old_y, blast.y) + _SW_LASER_HALF_H
            if _enemy_laser_hits_duck(blast.x, y_min, y_max, duck_x_min, duck_x_max, duck_y_min, duck_y_max):
                self.game_over = True
                return
            enemy_lasers_next.append(blast)
        self.enemy_lasers = enemy_lasers_next

        for star in self.stars:
            star.y -= star.speed
            if star.y < 0:
                star.y = _SW_HEIGHT
                star.x = random.uniform(0, _SW_WIDTH)

        for enemy in self.enemies:
            if _duck_hits_enemy(duck_x_min, duck_x_max, duck_y_min, duck_y_max, enemy):
                self.game_over = True
                return

    def _draw(self) -> None:
        ax = self.axes
        ax.cla()
        ax.set_xlim(0, _SW_WIDTH)
        ax.set_ylim(0, _SW_HEIGHT)
        ax.set_xticks([])
        ax.set_yticks([])
        ax.set_facecolor("#050510")

        for star in self.stars:
            size = 0.04 + star.speed * 0.08
            ax.add_patch(Circle((star.x, star.y), radius=size, facecolor="#ccccff", edgecolor="none", alpha=0.7))

        for enemy in self.enemies:
            _draw_tie_fighter(ax, enemy.x, enemy.y)
        for blast in self.enemy_lasers:
            _draw_enemy_laser(ax, blast.x, blast.y)
        for laser in self.lasers:
            _draw_laser(ax, laser.x, laser.y)
        _draw_duck_ship(ax, self.duck_x, self.duck_y)

        ax.text(0.5, _SW_HEIGHT - 0.8, f"score: {self.score}", fontsize=11, color="#ffee88")
        ax.text(
            0.5,
            0.5,
            "arrows / WASD: move    space: shoot",
            fontsize=9,
            color="#888899",
        )
        if self.game_over:
            ax.text(
                _SW_WIDTH / 2,
                _SW_HEIGHT / 2,
                "GAME OVER\npress any key to try again",
                ha="center",
                va="center",
                fontsize=14,
                color="#ff4444",
                weight="bold",
            )


class FlappyDuckModule(opendaq.Module):
    _instance: "FlappyDuckModule | None" = None

    def __init__(self, context: opendaq.IContext) -> None:
        super().__init__(context, name="FlappyDuckModule", version=(1, 0, 0), id="FlappyDuckModuleId")
        FlappyDuckModule._instance = self
        self._keyboard_delegates: dict[str, KeyboardFb] = {}

    @classmethod
    def keyboard_delegate(cls, local_id: str) -> KeyboardFb | None:
        if cls._instance is None:
            return None
        return cls._instance._keyboard_delegates.get(local_id)

    def on_get_available_function_block_types(self) -> "dict[str, opendaq.IFunctionBlockType]":
        return {
            "FlappyDuckFb": FlappyDuckFb.create_function_block_type(),
            "StarWarsDuckFb": StarWarsDuckFb.create_function_block_type(),
            "KeyboardFb": KeyboardFb.create_function_block_type(),
        }

    def on_create_function_block(
        self,
        id: str,
        parent: opendaq.IComponent,
        local_id: str,
        config: opendaq.IPropertyObject = None,
    ) -> opendaq.FunctionBlock | None:
        if id == "FlappyDuckFb":
            return FlappyDuckFb(self.context, parent, local_id)
        if id == "StarWarsDuckFb":
            return StarWarsDuckFb(self.context, parent, local_id)
        if id == "KeyboardFb":
            fb = KeyboardFb(self.context, parent, local_id)
            self._keyboard_delegates[local_id] = fb
            return fb
        return None


def create_module(context: "opendaq.IContext") -> FlappyDuckModule:
    return FlappyDuckModule(context)


def main():
    game = "flappy"
    if len(sys.argv) > 1:
        arg = sys.argv[1].lower()
        if arg in ("starwars", "sw", "star"):
            game = "starwars"
        elif arg in ("flappy", "fb"):
            game = "flappy"

    instance_builder = opendaq.InstanceBuilder()
    instance_builder.using_scheduler_main_loop = True
    instance = instance_builder.build()

    instance.module_manager.load_python_module(create_module(instance.context))

    if game == "starwars":
        instance.add_function_block("StarWarsDuckFb")
        print("Star Wars Duck - click the window, arrows/WASD to move, space to shoot.\n")
    else:
        instance.add_function_block("FlappyDuckFb")
        print("Flappy Duck - click the game window and press any key to flap.\n")

    print("Close the window or Ctrl+C to quit.\n")
    while True:
        sleep(0.01)
        instance.context.scheduler.run_main_loop_iteration()


if __name__ == "__main__":
    main()
