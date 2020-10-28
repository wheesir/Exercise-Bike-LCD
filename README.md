# Exercise-Bike-LCD

Imgur Album: https://imgur.com/gallery/aOrw7Yx

Inspiration and some code borrowed from: https://www.youtube.com/watch?v=emS56uT_-zQ

Bought a exercise bike off Amazon and like most, the display that it comes with is pretty underwhelming. I wanted something that showed some different stats as well as the current resistance and some kind of total output for the workout so I could compare them to previous workouts. I also wanted an auto-pause since I frequently start a ride and forget something like a water bottle or towel. 

The RPM sensor that was already on the bike (I think either a hall effect or reed switch) connected with a 3.5mm headphone jack to the original display. Hooked that to a digital interrupt pin on the Arduino. For the resistance, I mounted a slide potentiometer to the frame and attached the slider to the magnetic piece that moves up and down when you turn the resistance knob. It had a convenient hole in it already which the nub of the slider fit perfectly into. Added some superglue just to ensure a tight fit and remove any wiggle. Also attached this to an Arduino analog input pin through a 3.5mm headphone jack which makes for no permanent connections in case I want to change either the bike or LCD in the future.

The info on the readout is:
1: Current "power" 0-100 with bar graph. Displays up to 99 since I didn't want to use an extra space for the 3rd digit that I'll never see!
2: Average power for this ride/total power & current RPM. Used the x-bar for average and sigma for total to save space on the LCD. Didn't want it getting to cluttered.
3:Average MPH this ride & current MPH. Speed is calculated from the flywheel circumference and time between revolutions, not the pedal RPM so it probably shows higher than what you'd be doing on an actual bike. I think this is how most exercise bikes report speed though.
4:Total Miles this ride, time elapsed, and current resistance setting 

It auto-pauses after 2 seconds of no RPM readings and flashes "PAUSE" alternating with the time elapsed. Will pick up where it left off once it senses a spin again.
I had no way to measure the actual watt output so made up my own power calculation: (RPM*Resistance)/100. Seems to give me what I was looking for to compare rides and see an instant readout for how much work you're doing at different speeds and resistance levels. For total power, I basically used the kilojoules equation: (watts*seconds)/1000. Replace watts with my made up power number.

I thought of having some kind of Wi-Fi upload to save data, but for now am just using a Google Form I set up to manually enter the data on my phone after a workout and save to a spreadsheet. This has been working great and allows me to capture more data like the type of ride I did and any free-text I want to add.
