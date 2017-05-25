# LEDPillar

See this page for more information [https://blog.abluestar.com/projects/2017-led-pillar](https://blog.abluestar.com/projects/2017-led-pillar)

The LED Pillar project is thought experiment of how to make a game with a single strip of LEDs as the display. After a few different iterations the final game resembles popular [Guitar Hero](https://en.wikipedia.org/wiki/Guitar_Hero) game from 2005 on a single LED strip. 
 
The object of this game is to score as many points as possible by pressing the corresponding colored button as a “beat” enters the goal area. If a beat leaves the goal area or the user presses the wrong colored button, they lose a life. The user starts with 5 lives. As the game progresses the speed of the “beats” increases
 
This game consists of a strip of individually addressable RGB LEDs as the display, three big colored arcade buttons for user input, and LED Matrix for the score board and a microprocessor (arduino) to wire everything together. The LED strip is attached to a 5 Meter tall metal electrical conduit pipe and wrapped in bubble wrap as a diffuser. The LED pillar towers overtop of the player. 
 
This project was originally made for the [Vancouver Mini Maker faire 2017](http://vancouver.makerfaire.com/). Full source code for this project can be found in my github. [LEDPillar Source code](https://github.com/funvill/LEDPillar)
 
## Parts list 

- Individually addressable RGB LED. [WS2812 60 LEDs per meter](https://www.aliexpress.com/item/1m-4m-5m-WS2812B-Smart-led-pixel-strip-Black-White-PCB-30-60-144-leds-m/2036819167.html)
- [Arduino nano](https://www.arduino.cc/en/Main/arduinoBoardNano)
- [Arduino nano screw terminal shield](https://www.aliexpress.com/item/Nano-Terminal-Adapter-Screw-Shield-NANO-IO-Shield/32572673304.html)
- Three [100MM LED arcade push button](https://www.aliexpress.com/item/Free-shipping-5pcs-100MM-LED-Light-Lamp-Arcade-push-button-Big-Round-Arcade-Video-Game-Player/32655681463.html)
- Two [10ft 1 inch Conduit](https://www.homedepot.ca/en/home/p.1-inch-emt-conduit.1000106372.html)
- Four [adjustable pipe clamps](https://www.lowes.com/pd/Murray-10-Pack-3-4-in-1-1-2-in-Dia-Stainless-Steel-Adjustable-Clamps/50069657)


![Fritzing Drawing](/hardware/LEDPillar_bb.png)

