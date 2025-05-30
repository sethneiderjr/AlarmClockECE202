- This alarm clock project uses a Nokia 5110 LCD screen, and includes Adafruit libraries to connect and write to the screen. Three buttons are used to set the current time and alarm manually.
- Three more buttons are used to turn the alarm off in a set sequence.
- LED pin and Piezo Alarm pin turns on when the alarm boolean variable is true.
- Motors turn on when alarm boolean is true, and the circuit uses an Hbridge to give enough current to power them.
- PIR Motion sensor detects any motion to speed up the motors by giving a higher PWM signal.
- UltraSonic sensor detects objects by sending out high frequency noise that echoes back from an object in front of it.
- Sound calculations determine how far the object in front is, and if the distance is too low the motors will back up, with one side spinning faster to allow the alarm clock to turn.
- Turning off the alarm from the buttons snoozes the alarm time, and forces it to set the alarm time one minute into the future.
- This happens two times after the initial alarm set, making the alarm go off a total of three times from one alarm set.
