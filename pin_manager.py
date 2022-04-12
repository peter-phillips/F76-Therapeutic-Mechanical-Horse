from machine import Pin, ADC


class PinManager:
    def __init__(self):
        self.__set_up_pins()

    def __set_up_pins(self):
        self.led = Pin(5, Pin.OUT)

        self.button = Pin(34, Pin.IN)
        self.button_power = Pin(32, Pin.OUT)
        # button high will be 3.3V
        self.button_power.value(1)

        self.pot = ADC(Pin(33))
        self.pot.atten(ADC.ATTN_11DB)

        # range of values for pot positions 0 is all left
        self.pos_range = [[4095, 4095], [4094, 3850], [3849, 3040],
                     [3039, 2510], [2509, 2090], [2089, 1680],
                     [1679, 1250], [1249, 780], [779, 380], [379, 0]]

    def read_pot(self, pot_pos):
        val = self.pot.read()

        # if value is outside of range, find new position
        if val > self.pos_range[pot_pos][0] or self.pos_range[pot_pos][1] > val:
            for i in range(0, len(self.pos_range)):
                # if value is in the position, return the new position
                if self.pos_range[i][1] <= val <= self.pos_range[i][0]:
                    return i

            print(pot_pos, val)

        return pot_pos

    def button_press(self):
        if self.button.value() > 0:
            # button not pressed
            self.led.value(0)  # led on
            return True
        else:
            # button pressed
            self.led.value(1)  # led off
            return False
