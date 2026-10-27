# The first hour at the robot — SC2026

Print this. Work down it in order. **Do not skip ahead** — each step exists
because the next one is impossible to debug without it.

Write every number in the boxes. "It works" is not a result;
"commanded 1000 mm, measured 985, −1.5%" is.

Everything is done in the browser on **`tools/settings.html`** (saved on the
robot) and **`tools/controlpanel.html`**. Nothing to edit in code. Beginner?
The same path with screenshots-in-words is in `BEGINNER-GUIDE.html`.

---

## 0 · Before you plug anything in · 10 min

- [ ] Robot on blocks, wheels off the ground
- [ ] Battery charged, voltage recorded: `________ V`
- [ ] Laptop on the robot's Wi-Fi (`WorldSkills-1234`, password `password`)
- [ ] Laptop address starts `10.12.34.` (`ipconfig | findstr IPv4`)
- [ ] `ping 10.12.34.2` answers

```bash
ssh pi@10.12.34.2
```
Password `raspberry`.

---

## 1 · Get the program running · 15 min

On the laptop (not in the SSH window):

```bash
scp -r robot-onboard pi@10.12.34.2:~/
```

On the robot:

```bash
sudo date -s "YYYY-MM-DD HH:MM:SS"     # real date and time
cd ~/robot-onboard
make -j4
chmod +x run.sh
./run.sh
```

- [ ] Build finished with no "Error"
- [ ] Log shows `navX-Sensor Connected`
- [ ] Log shows `panel server: LISTENING on port 5800`

Open `tools/settings.html`, robot `10.12.34.2`, port `5800`.

- [ ] Page says **connected**
- [ ] Lift height reads about 40 mm (if it reads a huge number, note it: `________`)

---

## 2 · Which wheel is which · 15 min — on blocks

On the Settings page press each **Spin** button once. It spins ONE wheel
forward for a second at low power.

| Button | Wheel that actually moved | Turned forward? |
|---|---|---|
| Spin front-left | | |
| Spin front-right | | |
| Spin rear-left | | |
| Spin rear-right | | |

- [ ] Wrong wheel moved → change that wheel's **motor port** (Wiring section), press **Restart robot program**, test again
- [ ] Right wheel, wrong direction → tick its **reversed** box (Motor directions). Applies instantly.
- [ ] Lift: **Lift up** goes up. If not, tick **Lift reversed**.

Write down the final motor-port table (also saved on the robot):

| | FL | FR | RL | RR | Lift |
|---|---|---|---|---|---|
| Port | | | | | |

**Never** fix a wrong direction by swapping how you press a button. The
reversed box keeps driving and odometry consistent.

---

## 3 · The number everything else inherits · 15 min

- [ ] Wheel radius, measured with calipers: `________ mm`
- [ ] Enter it in Settings → Drivetrain → Wheel radius
- [ ] Drive **Forward 1000 mm** (guide Step 12, or the control panel). Measured: `________ mm`
- [ ] Type the measured distance into **Calibration helpers → Wheel size** and press **Apply new wheel radius**
- [ ] Drive again. Measured: `________ mm`

**Do not continue until this is within 2%.** Every distance the robot ever
computes is this number multiplied by something.

---

## 4 · Mecanum direction check — still on blocks · 10 min

From the control panel, send each command and watch the wheels:

- [ ] forward — all four wheels turn "forward"
- [ ] strafe left — an X pattern (FL and RR one way, FR and RL the other)
- [ ] turn — the left side opposite the right

A sign error at floor level launches the robot sideways into a table leg.

---

## 5 · Wheels down, first real move · 15 min

From `tools/controlpanel.html`, send `forward:1000` and measure with a tape.

| Run | Commanded | Measured | Error |
|---|---|---|---|
| 1 | 1000 mm | | |
| 2 | 1000 mm | | |
| 3 | 1000 mm | | |

- [ ] Within ±20 mm, **three runs in a row**

One good run is luck. Then repeat on a low battery and watch what changes —
that is why closed-loop control exists.

---

## 6 · Clamp and lift · 15 min

- [ ] `clamp_open` and `clamp_hold` from the panel both give the expected arm position. If not, Settings → Clamp servo → **Open angle** / **Hold angle**. Applies instantly.
- [ ] Lift scale: send the lift to 100 mm, measure the real fork height `________ mm`, use **Calibration helpers → Lift scale**.
- [ ] `lift_to:40` reaches carry height without grinding at the top or bottom.
- [ ] `lift_home` returns to 40 mm reliably.

---

## What to send the team lead at the end of the hour

1. The motor-port table from Step 2.
2. The wheel radius and the three 1000 mm results.
3. The clamp angles and the lift scale.
4. Your changed settings: Settings page → **Copy my settings**, paste into a message. This is your whole calibration in one block, and it restores a new SD card in seconds (**Apply pasted settings**).

---

## If it goes wrong

**Motors do nothing.** CAN wiring and the Titan status light first. Then
Settings → Wiring: are the Titan CAN IDs right (drive 42, lift 43 by default)?
Then check the stop button: it is active-low, so an unpressed button reads
*true*.

**Settings page says "not reachable" but SSH works.** Is `./run.sh` still
running? Is the laptop on the robot's Wi-Fi (`10.12.34.x`), and is the
robot address `10.12.34.2` (not your laptop's)? Try a private window: a
browser proxy or extension can block local addresses.

**`make` says "up to date" after copying files.** The robot clock is 2022.
Set the date, then `make -B -j4`.

**`cannot bind port`.** Another copy is running: `pkill robot`.

**The robot drives sideways when told to go forward.** Motor ports don't
match the physical wheels. Redo Step 2: both halves look right in
isolation; only the combination is wrong.

**Lift height is a huge number.** The lift encoder is not zeroed or on the
wrong port. Check Settings → Wiring → Lift motor port.

**Nothing makes sense.** Go back to Step 1 and work down again. The first
step that fails is the real fault; everything after it is a symptom.
