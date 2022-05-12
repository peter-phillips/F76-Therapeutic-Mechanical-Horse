from pin_manager import *
from wifi_manager import *

pin_manager = PinManager()
wifi_manager = WiFiManager()

horse_on = False
just_changed = change_cyc = 1000
pot_pos = 0
while True:
    pot_pos = pin_manager.read_pot(pot_pos)
    pressed = pin_manager.button_press()

    if just_changed == 0 and pressed:
        if horse_on:
            pin_manager.led.value(1)
            wifi_manager.send_code("off_h")

        if not horse_on:
            pin_manager.led.value(0)
            wifi_manager.send_code("on_h")

        horse_on = not horse_on
        just_changed = change_cyc

    elif not pressed:
        if just_changed > 0:
            just_changed -= 1










































































































