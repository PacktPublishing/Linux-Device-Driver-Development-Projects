import tkinter as tk
import subprocess

SSH_TARGET  = "qemu-vm"
LED_BASE    = "/sys/class/leds"
POLL_MS     = 10
RECONNECT_MS = 5000
CIRCLE_R    = 60   # radius in pixels
CELL_W      = 180
CELL_H      = 210
COLS        = 2

# Map color keywords found in LED names to (off, on) fill colors.
COLOR_MAP = {
    "red":    ("#180000", "#ff3030"),
    "green":  ("#001800", "#40e040"),
    "blue":   ("#000018", "#3090ff"),
    "yellow": ("#181800", "#ffff30"),
    "orange": ("#180a00", "#ff9020"),
    "amber":  ("#180c00", "#ffb020"),
    "white":  ("#1a1a1a", "#ffffff"),
    "violet": ("#0c0018", "#cc60ff"),
    "cyan":   ("#001818", "#40ffff"),
}

def led_color(name):
    """Extract a known color keyword from an LED name like 'green:status'."""
    for part in name.split(":"):
        if part in COLOR_MAP:
            return part
    return "white"

def brightness_fill(color_name, brightness, max_brightness):
    """Interpolate between off and on color based on brightness ratio."""
    off_hex, on_hex = COLOR_MAP.get(color_name, ("#505050", "#ffffff"))
    ratio = brightness / max_brightness if max_brightness else 0

    def parse(h):
        h = h.lstrip("#")
        return int(h[0:2], 16), int(h[2:4], 16), int(h[4:6], 16)

    r0, g0, b0 = parse(off_hex)
    r1, g1, b1 = parse(on_hex)
    r = int(r0 + (r1 - r0) * ratio)
    g = int(g0 + (g1 - g0) * ratio)
    b = int(b0 + (b1 - b0) * ratio)
    return f"#{r:02x}{g:02x}{b:02x}"

# --- Discover LEDs at startup ---
discover = subprocess.run(
    ["ssh", SSH_TARGET, f"ls {LED_BASE}/"],
    capture_output=True, text=True,
)
led_names = discover.stdout.split()
if not led_names:
    raise SystemExit("No LEDs found under /sys/class/leds/ on the target")

rows = (len(led_names) + COLS - 1) // COLS
win_w = CELL_W * min(len(led_names), COLS)
win_h = CELL_H * rows

# --- Build UI ---
root = tk.Tk()
root.title("QEMU LED Viewer")
root.resizable(False, False)

canvas = tk.Canvas(root, width=win_w, height=win_h, bg="#1e1e1e")
canvas.pack(padx=8, pady=8)

cx = CELL_W // 2
cy = CIRCLE_R + 20

led_items = {}  # name -> {circle, label_val, label_max}

for idx, name in enumerate(led_names):
    col = idx % COLS
    row = idx // COLS
    x   = col * CELL_W + CELL_W // 2
    y   = row * CELL_H + CIRCLE_R + 20

    color = led_color(name)
    off_c, _ = COLOR_MAP.get(color, ("#505050", "#ffffff"))

    circle = canvas.create_oval(
        x - CIRCLE_R, y - CIRCLE_R,
        x + CIRCLE_R, y + CIRCLE_R,
        fill=off_c, outline="#888888", width=2,
    )
    canvas.create_text(
        x, y + CIRCLE_R + 14,
        text=name, fill="#dddddd",
        font=("Courier", 9, "bold"),
    )
    label_val = canvas.create_text(
        x, y + CIRCLE_R + 30,
        text="0 / ?", fill="#aaaaaa",
        font=("Courier", 9),
    )

    led_items[name] = {"circle": circle, "label_val": label_val, "color": color}

# --- Persistent SSH loop ---
# One SSH session; outputs a block of "name brightness max_brightness" lines
# terminated by "---" per poll cycle.
ssh_cmd = (
    "while true; do "
    f"  for d in {LED_BASE}/*/; do "
    '    name=$(basename "$d"); '
    '    b=$(cat "$d/brightness" 2>/dev/null || echo 0); '
    '    m=$(cat "$d/max_brightness" 2>/dev/null || echo 1); '
    '    printf "%s %s %s\\n" "$name" "$b" "$m"; '
    "  done; "
    "  echo '---'; "
    "  sleep 0.1; "
    "done"
)

def start_ssh():
    return subprocess.Popen(
        ["ssh", SSH_TARGET, ssh_cmd],
        stdout=subprocess.PIPE,
        stderr=subprocess.DEVNULL,
        text=True,
    )


ssh = start_ssh()

pending = {}

def update():
    try:
        while True:
            line = ssh.stdout.readline()
            if not line:
                raise EOFError("SSH stream closed")
            line = line.strip()

            if line == "---":
                # Full frame received — update UI
                for name, vals in pending.items():
                    if name not in led_items:
                        continue
                    b, m     = vals
                    item     = led_items[name]
                    color    = item["color"]
                    fill     = brightness_fill(color, b, m)
                    canvas.itemconfig(item["circle"],    fill=fill)
                    canvas.itemconfig(item["label_val"], text=f"{b} / {m}")
                pending.clear()
                break  # yield back to tkinter; next frame on next after()

            parts = line.split()
            if len(parts) == 3:
                name, b, m = parts
                try:
                    pending[name] = (int(b), int(m))
                except ValueError:
                    pass

    except EOFError:
        for item in led_items.values():
            canvas.itemconfig(item["circle"],    fill="#444444")
            canvas.itemconfig(item["label_val"], text="SSH lost")
        root.after(RECONNECT_MS, reconnect)
        return

    except Exception as e:
        for item in led_items.values():
            canvas.itemconfig(item["circle"],    fill="#444444")
            canvas.itemconfig(item["label_val"], text="ERR")
        print(e)
        root.after(RECONNECT_MS, reconnect)
        return

    root.after(POLL_MS, update)

def reconnect():
    global ssh

    try:
        ssh = start_ssh()
        update()
    except Exception:
        root.after(RECONNECT_MS, reconnect)


def on_close():
    if ssh:
        ssh.terminate()
        ssh.wait()
    root.destroy()

root.protocol("WM_DELETE_WINDOW", on_close)
update()
root.mainloop()
